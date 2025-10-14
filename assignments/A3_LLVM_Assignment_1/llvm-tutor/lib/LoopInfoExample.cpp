// LoopInfoExample.cpp
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  static void printLoopInfo(Loop *L, unsigned Depth) {
    std::string Indent(Depth * 2, ' ');

    //errs() << Indent << "Loop at depth " << Depth << ":\n";
    errs() << Indent << "Loop at depth " << L->getLoopDepth()
         << " (indent depth=" << Depth << ")\n";

    // Print header
    errs() << Indent << "  Header: ";
    L->getHeader()->printAsOperand(errs(), false);
    errs() << "\n";

    // Print preheader
    if (BasicBlock *Preheader = L->getLoopPreheader()) {
      errs() << Indent << "  Preheader: ";
      Preheader->printAsOperand(errs(), false);
      errs() << "\n";
    } else {
      errs() << Indent << "  Preheader: <none>\n";
    }

    // Print latch
    if (BasicBlock *Latch = L->getLoopLatch()) {
      errs() << Indent << "  Latch: ";
      Latch->printAsOperand(errs(), false);
      errs() << "\n";
    } else {
      errs() << Indent << "  Latch: <multiple or none>\n";
    }

    // Print exiting blocks
    SmallVector<BasicBlock*, 4> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);
    errs() << Indent << "  Exiting blocks: " << ExitingBlocks.size() << "\n";
    for (BasicBlock *EB : ExitingBlocks) {
      errs() << Indent << "    ";
      EB->printAsOperand(errs(), false);
      errs() << "\n";
    }
    
    // Print exit blocks
    SmallVector<BasicBlock*, 4> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    errs() << Indent << "  Exit blocks: " << ExitBlocks.size() << "\n";
    for (BasicBlock *EB : ExitBlocks) {
      errs() << Indent << "    ";
      EB->printAsOperand(errs(), false);
      errs() << "\n";
    }

    // Recursively print subloops
    if (!L->getSubLoops().empty()) {
      errs() << Indent << "  Subloops: " << L->getSubLoops().size() << "\n";
      for (Loop *SubL : L->getSubLoops()) {
        printLoopInfo(SubL, Depth + 1);
      }
    } else {
      errs() << Indent << "  Subloops: none\n";
    }
    
    errs() << "\n";
  }


struct LoopInfoExample : public PassInfoMixin<LoopInfoExample> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    errs() << "Analyzing function: " << F.getName() << "\n";

    // Get LoopInfo for this function
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    for (Loop *L : LI) {
      printLoopInfo(L, 1);
      // errs() << "  Found a loop with header: ";
      // L->getHeader()->printAsOperand(errs(), false);
      // errs() << "\n";

      // // Print subloops
      // for (Loop *SubL : L->getSubLoops()) {
      //   errs() << "    Subloop with header: ";
      //   SubL->getHeader()->printAsOperand(errs(), false);
      //   errs() << "\n";
      // }
    }

    return PreservedAnalyses::all();
  }
};
} // namespace

// Register pass plugin
llvm::PassPluginLibraryInfo getLoopInfoExamplePluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "loop-info-example", LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-info-example") {
                FPM.addPass(LoopInfoExample());
                return true;
              }
              return false;
            });
      }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoopInfoExamplePluginInfo();
}

