// DeadStoreElimination.cpp
// Intraprocedural DSE using MemorySSA

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/PostDominators.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

namespace {

struct DeadStoreEliminationPass : PassInfoMixin<DeadStoreEliminationPass> {

  // identify removable stores 
  static bool isRemovableStore(const Instruction *I) {
    const auto *SI = dyn_cast<StoreInst>(I);
    if (!SI) return false;
    if (SI->isVolatile()) return false;
    if (SI->isAtomic())   return false;
    return true;
  }

  // --- Helper: walk the MemorySSA chain between two MemoryDefs and see if
  //             there’s any intervening read of the earlier store’s location.
  //
  // Earlier = the earlier MemoryDef (the candidate "dead" store)
  // Later   = the later  MemoryDef (the "killer" store)
  //
  // We start from Later and walk defining access edges backward until we reach
  // Earlier (or we bail). If we encounter a MemoryUse that aliases the location
  // written by Earlier, we report an intervening use -> cannot DSE.
  //
  // Notes:
  //  * If we cross a MemoryPhi we bail (paths merged).
  static bool hasInterveningUseMSSA(const MemoryDef *Earlier,
                                    const MemoryDef *Later,
                                    AAResults &AA) {
    // Must have valid store instructions to compute MemoryLocation.
    const auto *EarlierStore = dyn_cast<StoreInst>(Earlier->getMemoryInst());
    if (!EarlierStore) return true; 

    MemoryLocation EarlierLoc = MemoryLocation::get(EarlierStore);

    const MemoryAccess *MA = Later;
    // Step one hop back so we examine accesses that are between Earlier and Later.
    if (const auto *MD = dyn_cast<MemoryDef>(MA))
      MA = MD->getDefiningAccess();

    while (MA && MA != Earlier) {
      if (const auto *MU = dyn_cast<MemoryUse>(MA)) {
        const Instruction *UserI = MU->getMemoryInst();

        // If the use is a load, check alias against the earlier store location.
        if (const auto *LI = dyn_cast_or_null<LoadInst>(UserI)) {
          MemoryLocation LoadLoc = MemoryLocation::get(LI);
          if (AA.alias(LoadLoc, EarlierLoc) != AliasResult::NoAlias)
            return true; // intervening read of the same/may alias location
        } else {
          // Other kinds of MemoryUse (e.g., calls modeled as uses) 
          return true;
        }
      } else if (isa<MemoryPhi>(MA)) {
        // Merged flows
        return true;
      }

      if (const auto *MD2 = dyn_cast<MemoryDef>(MA)) {
        MA = MD2->getDefiningAccess();
      } else {
        break;
      }
    }

    // If we cleanly reached Earlier without finding a conflicting MemoryUse
    return false;
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &AA   = AM.getResult<AAManager>(F);
    auto &MSSAR = AM.getResult<MemorySSAAnalysis>(F);
    MemorySSA &MSSA = MSSAR.getMSSA();
    auto &PDT  = AM.getResult<PostDominatorTreeAnalysis>(F);

    MemorySSAUpdater Updater(&MSSA);

    errs() << "Running Dead Store Elimination on function: " << F.getName() << "\n";
    errs() << "========================================\n";

    // Collect all MemoryDefs for a reverse walk.
    SmallVector<MemoryDef*, 32> MemDefs;
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *MA = MSSA.getMemoryAccess(&I))
          if (auto *MD = dyn_cast<MemoryDef>(MA))
            MemDefs.push_back(MD);
      }
    }

    // A set to avoid double-erasing the same instruction if multiple later
    // stores "kill" the same earlier store.
    SmallPtrSet<Instruction*, 16> Dead;

    // Walk MemoryDefs in reverse "program order".
    for (auto *D : llvm::reverse(MemDefs)) {
      Instruction *Inst = D->getMemoryInst();

      // We only care about *stores* as potential killers.
      if (!isRemovableStore(Inst))
        continue;
      auto *SI = cast<StoreInst>(Inst);

      errs() << "Examining store: ";
      Inst->print(errs());
      errs() << "\n";

      // Use the MemorySSA walker to find what this store clobbers (previous write).
      auto *Walker = MSSA.getWalker();
      MemoryAccess *Prev = Walker->getClobberingMemoryAccess(D);

      errs() << "  Clobbering access: ";
      Prev->print(errs());
      errs() << "\n";

      // Only proceed if the clobber is another MemoryDef (i.e. a previous write).
      auto *PrevDef = dyn_cast<MemoryDef>(Prev);
      if (!PrevDef) {
        errs() << "  -> Not a MemoryDef (no prior write), skipping\n";
        continue;
      }

      Instruction *PrevInst = PrevDef->getMemoryInst();
      auto *PrevSI = dyn_cast<StoreInst>(PrevInst);
      if (!PrevSI) {
        errs() << "  -> Previous instruction is not a store, skipping\n";
        continue;
      }

      // Skip non-removable stores (volatile/atomic).
      if (!isRemovableStore(PrevSI)) {
        errs() << "  -> Previous store is volatile/atomic, skipping\n";
        continue;
      }

      errs() << "  Previous store: ";
      PrevInst->print(errs());
      errs() << "\n";

      // Require exact same location: MustAlias + same size.
      MemoryLocation LocNew = MemoryLocation::get(SI);
      MemoryLocation LocOld = MemoryLocation::get(PrevSI);
      AliasResult AR = AA.alias(LocNew, LocOld);
      if (AR != AliasResult::MustAlias || LocNew.Size != LocOld.Size) {
        errs() << "  -> Not MustAlias and same-size; skipping (AR=" << (int)AR << ")\n";
        continue;
      }

      // Intervening use on any path (via MSSA chain)? If yes, we cannot remove.
      if (hasInterveningUseMSSA(PrevDef, D, AA)) {
        errs() << "  -> Has intervening use (MSSA chain), skipping\n";
        continue;
      }

      // Post-dominance: the later store must post-dominate the earlier write's BB,
      // ensuring the earlier store is always overwritten on all paths.
      if (!PDT.dominates(SI->getParent(), PrevInst->getParent())) {
        errs() << "  -> Later store does not post-dominate earlier store, skipping\n";
        continue;
      }

      // Mark earlier store dead.
      errs() << "  -> Earlier store is DEAD and will be removed: ";
      PrevInst->print(errs());
      errs() << "\n";

      Dead.insert(PrevInst);
    }

    // Apply deletions and update MemorySSA accordingly.
    unsigned NumDead = 0;
    for (Instruction *DeadStore : Dead) {
      errs() << "Removing dead store: ";
      DeadStore->print(errs());
      errs() << "\n";

      if (auto *MA = MSSA.getMemoryAccess(DeadStore))
        Updater.removeMemoryAccess(MA);

      DeadStore->eraseFromParent();
      ++NumDead;
    }

    errs() << "\n========================================\n";
    errs() << "Dead Store Elimination completed!\n";
    errs() << "Total dead stores eliminated: " << NumDead << "\n";
    errs() << "========================================\n\n";

    // If we modified anything, be conservative about preserved analyses.
    if (NumDead > 0)
      return PreservedAnalyses::none();

    return PreservedAnalyses::all();
  }
};

} 

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION,
    "dse-mssa",               // Plugin name
    "v0.2",                   // Version
    [](PassBuilder &PB) {
      // Ensure analyses we rely on are registered.
      PB.registerAnalysisRegistrationCallback(
        [](FunctionAnalysisManager &FAM) {
          FAM.registerPass([] { return MemorySSAAnalysis(); });
          FAM.registerPass([] { return PostDominatorTreeAnalysis(); });
          FAM.registerPass([] { return AAManager(); });
        });

      // Pipeline hook: -passes="require<memoryssa>,dse"
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "dse") {
            FPM.addPass(DeadStoreEliminationPass());
            return true;
          }
          return false;
        });
    }
  };
}
