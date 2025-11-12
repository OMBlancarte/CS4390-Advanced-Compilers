#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

struct MemorySSADemoPass : PassInfoMixin<MemorySSADemoPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &MSSAResult = AM.getResult<MemorySSAAnalysis>(F);
    auto &MSSA = MSSAResult.getMSSA();

    errs() << "Analyzing function: " << F.getName() << "\n";

    // Iterate over basic blocks to show all MemoryAccesses
    for (auto &BB : F) {
      errs() << "BasicBlock: " << BB.getName() << "\n";

      // MemoryPhi nodes are found at block entries
      if (auto *Phi = MSSA.getMemoryAccess(&BB)) {
        if (auto *MPhi = dyn_cast<MemoryPhi>(Phi)) {
          errs() << "  MemoryPhi for block " << BB.getName() << ":\n";
          for (unsigned i = 0; i < MPhi->getNumIncomingValues(); ++i) {
            auto *IncomingAcc = MPhi->getIncomingValue(i);
            auto *Pred = MPhi->getIncomingBlock(i);
            errs() << "    from " << Pred->getName() << ": ";
            IncomingAcc->print(errs());
            errs() << "\n";
          }
        }
      }

      // Iterate over instructions for MemoryDef/Use
      for (auto &I : BB) {
        if (auto *MA = MSSA.getMemoryAccess(&I)) {
          errs() << "  ";
          MA->print(errs());
          errs() << "\n";
        }
      }
    }

    return PreservedAnalyses::all();
  }
};

extern"C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MemorySSADemoPass", "v0.9",
          [](PassBuilder &PB) {
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([] { return MemorySSAAnalysis(); });
                });

            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "memssa-demo") {
                    FPM.addPass(MemorySSADemoPass());
                    return true;
                  }
                  return false;
                });
          }};
}

