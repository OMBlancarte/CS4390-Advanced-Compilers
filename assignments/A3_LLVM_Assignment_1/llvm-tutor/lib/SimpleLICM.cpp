/* SimpleLICM.cpp
 *
 * This pass hoists loop-invariant code before the loop when it is safe to do so.
 *
 * Compatible with New Pass Manage
*/

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/CFG.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

using namespace llvm;

struct SimpleLICM : public PassInfoMixin<SimpleLICM> {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR,
                        LPMUpdater &) {
    DominatorTree &DT = AR.DT;

    BasicBlock *Preheader = L.getLoopPreheader();
    if (!Preheader) {
      errs() << "No preheader, skipping loop\n";
      return PreservedAnalyses::all();
    }

    SmallPtrSet<Instruction *, 8> InvariantSet;
    bool Change = true;

    // Worklist algorithm to identify loop invariant instructions
    /*************************************/
    
    // Keep iterating until no new invariant instructions are found
    while (Change) {
      Change = false;
      
      // Iterate through all basic blocks in the loop
      for (BasicBlock *BB : L.blocks()) {
        // Iterate through all instructions in the basic block
        for (Instruction &I : *BB) {
          // Skip if already marked as invariant
          if (InvariantSet.count(&I))
            continue;
          
          // Skip terminators
          if (I.isTerminator()) 
            continue;

          // Skip memory operations (loads, stores, etc.)
          if (I.mayReadOrWriteMemory())
            continue;
          
          // Skip phi instructions
          if (isa<PHINode>(I))
            continue;
          
          // Check if all operands are loop invariant
          bool AllOperandsInvariant = true;
          for (Use &U : I.operands()) {
            Value *Operand = U.get();
            
            // Constants are always loop invariant
            if (isa<Constant>(Operand))
              continue;
            
            // If operand is an instruction
            if (Instruction *OpInst = dyn_cast<Instruction>(Operand)) {
              // Check if it's defined outside the loop
              if (!L.contains(OpInst->getParent())) {
                continue; // Loop invariant (defined outside)
              }
              // Check if it's already in our invariant set
              else if (InvariantSet.count(OpInst)) {
                continue; // Loop invariant (previously identified)
              }
              else {
                // Operand is not loop invariant
                AllOperandsInvariant = false;
                break;
              }
            }
            // If operand is an argument, it's loop invariant
            else if (isa<Argument>(Operand)) {
              continue;
            }
            // Unknown operand type
            else {
              AllOperandsInvariant = false;
              break;
            }
          }
          
          // If all operands are loop invariant, mark this instruction as invariant
          if (AllOperandsInvariant) {
            InvariantSet.insert(&I);
            Change = true; // We found a new invariant instruction
          }
        }
      }
    }
    /*************************************/ 

    // Actually hoist the instructions
    for (Instruction *I : InvariantSet) {
      if (isSafeToSpeculativelyExecute(I) && dominatesAllLoopExits(I, &L, DT)) {
        errs() << "Hoisting: " << *I << "\n";
        I->moveBefore(Preheader->getTerminator());
      }
    }

    return PreservedAnalyses::none();
  }

  bool dominatesAllLoopExits(Instruction *I, Loop *L, DominatorTree &DT) {
    SmallVector<BasicBlock *, 8> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    for (BasicBlock *EB : ExitBlocks) {
      if (!DT.dominates(I, EB))
        return false;
    }
    return true;
  }
};

llvm::PassPluginLibraryInfo getSimpleLICMPluginInfo() {
  errs() << "SimpleLICM plugin: getSimpleLICMPluginInfo() called\n";
  return {LLVM_PLUGIN_API_VERSION, "simple-licm", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "simple-licm") {
                    LPM.addPass(SimpleLICM());
                    return true;
                  }                  
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "SimpleLICM plugin: llvmGetPassPluginInfo() called\n";
  return getSimpleLICMPluginInfo();
}
