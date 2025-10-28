/* ExtendedDerivedIV.cpp 
 *
 * This pass detects derived induction variables in nested loops using ScalarEvolution.
 * Extended to handle inner loops of loop nests.
 *
 * Compatible with New Pass Manager
*/

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

class ExtendedDerivedIV
    : public PassInfoMixin<ExtendedDerivedIV> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    // Process all loops using a recursive helper
    for (Loop *L : LI) {
      analyzeLoopNest(L, SE, 1);
    }

    return PreservedAnalyses::all();
  }

private:
  // Recursive function to analyze nested loops
  void analyzeLoopNest(Loop *L, ScalarEvolution &SE, int depth) {
    std::string indent(depth * 2, ' ');
    
    // errs() << indent << "Analyzing loop at depth " << depth
    errs() << indent << "Loop at depth " << L->getLoopDepth()
        //  << " (indent depth=" << depth << ")\n"; 
           << " (header: " << L->getHeader()->getName() << "):\n";

    BasicBlock *Header = L->getHeader();
    if (!Header) {
      errs() << indent << "  No header found, skipping\n";
      return;
    }

    // Analyze PHI nodes in this loop's header
    bool foundIV = false;
    for (PHINode &PN : Header->phis()) {
      if (!PN.getType()->isIntegerTy())
        continue;  
      const SCEV *S = SE.getSCEV(&PN);

      // Detect affine AddRec expressions
      if (auto *AR = dyn_cast<SCEVAddRecExpr>(S)) {
        const SCEV *Step = AR->getStepRecurrence(SE);
        const SCEV *Start = AR->getStart();

        // Check if it's affine
        if (AR->isAffine()) {
          foundIV = true;
          errs() << indent << "  Derived induction variable: " << PN.getName()
                << " = {" << *Start << ",+," << *Step << "}<"
                << L->getHeader()->getName() << ">\n";
        }
      }
    }

    if (!foundIV) {
      errs() << indent << "  No induction variables found in this loop\n";
    }

    // Recursively analyze inner loops (subloops)
    for (Loop *SubLoop : L->getSubLoops()) {
      analyzeLoopNest(SubLoop, SE, depth + 1);
    }
  }
};

} // namespace

// Register the pass
llvm::PassPluginLibraryInfo getExtendedDerivedIVPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ExtendedDerivedIV", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "extended-derived-iv") {
                    FPM.addPass(ExtendedDerivedIV());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getExtendedDerivedIVPluginInfo();
}