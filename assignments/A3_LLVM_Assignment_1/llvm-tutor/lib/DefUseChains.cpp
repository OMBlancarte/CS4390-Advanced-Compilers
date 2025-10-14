//
// DefUseChains: Print both Def→Use and Use→Def relationships for each
// instruction in a function.
//
// Compatible with LLVM 16–21 (new PassManager).
//

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct DefUseChains : public PassInfoMixin<DefUseChains> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "=============================\n";
    errs() << "Function: " << F.getName() << "\n";
    errs() << "=============================\n";

    for (auto &BB : F) {
      errs() << "\n" << "BasicBlock: ";
      BB.printAsOperand(errs(),false);
      errs() << "\n";

      for (auto &I : BB) {
        // --- DEF → USE ---
        errs() << "\n[DEF] " << I << "\n";

        if (I.use_empty()) {
          errs() << "   (no uses)\n";
        } else {
          for (const Use &U : I.uses()) {
            const User *Usr = U.getUser();
            if (const Instruction *UseInst = dyn_cast<Instruction>(Usr)) {
              errs() << "   [used in] " << *UseInst << "\n";
            }
          }
        }

        // --- USE → DEF ---
        errs() << "\n[USE]   [depends on defs:]\n";
        for (const Use &Op : I.operands()) {
          if (const Instruction *DefInst = dyn_cast<Instruction>(Op.get())) {
            errs() << "      " << *DefInst << "\n";
          } else if (const Argument *Arg = dyn_cast<Argument>(Op.get())) {
            errs() << "      (function argument) " << Arg->getName() << "\n";
          } else if (isa<Constant>(Op.get())) {
            errs() << "      (constant)\n";
          }
        }
      }
    }

    errs() << "\n";
    return PreservedAnalyses::all();
  }
};

} // namespace

//===----------------------------------------------------------------------===//
// Plugin registration
//===----------------------------------------------------------------------===//

llvm::PassPluginLibraryInfo getDefUseChainsPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "def-use-chains", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "def-use-chains") {
                    FPM.addPass(DefUseChains());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getDefUseChainsPluginInfo();
}

