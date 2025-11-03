// InductionVarElimination.cpp 
//  1. Identify basic induction variable i in SSA form:
//        i = phi [i0, preheader], [i_next, latch]
//        i_next = i + e       // e is a loop-invariant constant stride
//
//  2. Identify derived induction variables j that are affine in i:
//        j = c * i + d        // c,d are loop-invariant
//
//  3. Replace derived j with a newly constructed PHI node representing:
//        preheader: j0 = d
//        latch:     j_next = j + (c * e)
//        header:    j = phi [j0, preheader], [j_next, latch]
//
// Debug messages are printed at every major step (prefixed with [IVE-DEBUG]).
//
// Usage:
//   opt -load-pass-plugin ./lib/libInductionVarElimination.so \
//       -passes=induction-var-elimination \
//       -S -o outputs/matmul_ive.ll inputs/matmul_canonical.ll
//

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// ---------------------------------------------------------------------------
// Helper: Check if Num == Den * Quot exactly, for SCEVConstants.
// ---------------------------------------------------------------------------
static bool isExactMultiple(const SCEVConstant *Num,
                            const SCEVConstant *Den,
                            APInt &Quot) {
  const APInt &N = Num->getAPInt();
  const APInt &D = Den->getAPInt();

  if (D.isZero()) return false;

  APInt Q, R;
  APInt::sdivrem(N, D, Q, R);
  if (!R.isZero()) return false;

  Quot = Q;
  return true;
}

// Induction Variable Elimination
class InductionVarElimination : public PassInfoMixin<InductionVarElimination> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
    const DataLayout &DL = F.getParent()->getDataLayout();

    bool Changed = false;

    errs() << "\n[IVE-DEBUG] Running on function: " << F.getName() << "\n";

    // Process loops bottom-up
    for (Loop *TopLevel : LI) {
      Changed |= processLoopNest(*TopLevel, SE, DT, DL);
    }

    if (Changed)
      errs() << "[IVE-DEBUG] Changes applied to function: " << F.getName() << "\n";
    else
      errs() << "[IVE-DEBUG] No IVs eliminated in function: " << F.getName() << "\n";

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

private:
  bool processLoopNest(Loop &L,
                       ScalarEvolution &SE,
                       DominatorTree &DT,
                       const DataLayout &DL) {
    bool Changed = false;

    // Recurse first (bottom-up)
    for (Loop *Sub : L.getSubLoops())
      Changed |= processLoopNest(*Sub, SE, DT, DL);

    // Then transform current loop
    Changed |= eliminateInLoop(L, SE, DT, DL);
    return Changed;
  }

  bool eliminateInLoop(Loop &L,
                       ScalarEvolution &SE,
                       DominatorTree &DT,
                       const DataLayout &DL) {
    BasicBlock *Header    = L.getHeader();
    BasicBlock *Preheader = L.getLoopPreheader();
    BasicBlock *Latch     = L.getLoopLatch();

    if (!Header || !Preheader || !Latch) {
      errs() << "[IVE-DEBUG] Skipping loop (no canonical form: missing header/preheader/latch)\n";
      return false;
    }

    errs() << "\n[IVE-DEBUG] === Processing loop with header: ";
    Header->printAsOperand(errs(), false);
    errs() << " ===\n";

    // Find a basic IV 
    PHINode *BasicIV = nullptr;
    const SCEVAddRecExpr *ARI = nullptr;
    const SCEVConstant *StepI = nullptr;

    for (PHINode &PN : Header->phis()) {
      if (!PN.getType()->isIntegerTy()) continue;

      auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(&PN));
      if (!AR || !AR->isAffine() || AR->getLoop() != &L) continue;

      if (auto *StepC = dyn_cast<SCEVConstant>(AR->getStepRecurrence(SE))) {
        if (!BasicIV || StepC->getAPInt().isOne()) {
          BasicIV = &PN; ARI = AR; StepI = StepC;
          if (StepC->getAPInt().isOne()) break;
        }
      }
    }

    if (!BasicIV) {
      errs() << "[IVE-DEBUG] No basic IV found; skipping loop.\n";
      return false;
    }

    errs() << "[IVE-DEBUG] Basic IV found: ";
    BasicIV->printAsOperand(errs(), false);
    errs() << ", stride = " << StepI->getAPInt() << "\n";

    // Collect derived IVs
    SmallVector<PHINode*, 8> DerivedPHIs;
    for (PHINode &PN : Header->phis()) {
      if (&PN == BasicIV || !PN.getType()->isIntegerTy()) continue;

      auto *AR = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(&PN));
      if (AR && AR->isAffine() && AR->getLoop() == &L) {
        DerivedPHIs.push_back(&PN);
      }
    }

    if (DerivedPHIs.empty()) {
      errs() << "[IVE-DEBUG] No derived IVs detected for this loop.\n";
      return false;
    }

    errs() << "[IVE-DEBUG] Found " << DerivedPHIs.size() << " derived IV candidate(s).\n";

    SCEVExpander Expander(SE, DL, "ive_debug");
    const SCEV *Si = ARI->getStart();
    const SCEVConstant *eC = StepI;

    bool LoopChanged = false;

    // Process each derived IV 
    for (PHINode *JOld : DerivedPHIs) {
      errs() << "\n[IVE-DEBUG] Processing derived PHI: " << JOld->getName() << "\n";

      auto *ARJ = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(JOld));
      if (!ARJ) continue;

      const SCEV *Sj = ARJ->getStart();
      auto *SjStepC = dyn_cast<SCEVConstant>(ARJ->getStepRecurrence(SE));
      if (!SjStepC || !eC) {
        errs() << "[IVE-DEBUG]   Non-constant stride, skipping.\n";
        continue;
      }

      // Compute ratio c = SjStep / e
      APInt cAP;
      if (!isExactMultiple(SjStepC, eC, cAP)) {
        errs() << "[IVE-DEBUG]   Stride ratio not exact; skipping derived IV.\n";
        continue;
      }

      // Make sure c has the same bitwidth/type as JOld
      auto *JTy = cast<IntegerType>(JOld->getType());
      APInt cAPW = cAP.sextOrTrunc(JTy->getBitWidth());

      // Create loop-invariant constant SCEV for 'c' using APInt
      const SCEV *cS = SE.getConstant(cAPW);

      // Compute d = Sj - c * Si
      const SCEV *dS = SE.getMinusSCEV(Sj, SE.getMulExpr(cS, Si));
      if (!SE.isLoopInvariant(dS, &L)) {
        errs() << "[IVE-DEBUG]   Offset (d) not loop-invariant; skipping.\n";
        continue;
      }

      errs() << "[IVE-DEBUG]   Derived form: j = " << cAPW << " * i + d\n";

      // Expand d into concrete IR in the preheader:
      Value *j0 = Expander.expandCodeFor(
          dS,
          JOld->getType(),
          Preheader->getTerminator()
      );
      errs() << "[IVE-DEBUG]   Inserted preheader init j0 = d.\n";

      // Create new PHI in loop header:
      PHINode *JNew = PHINode::Create(
          JOld->getType(),
          /*NumReservedValues=*/2,
          JOld->hasName() ? (JOld->getName() + ".ive") : "j.ive",
          &*Header->getFirstNonPHI()
      );
      errs() << "[IVE-DEBUG]   Created PHI " << JNew->getName() << " in header.\n";

      // Compute per-iteration increment (c * e) as a constant of J's type.
      APInt ceAP = cAPW * eC->getAPInt();
      ceAP = ceAP.sextOrTrunc(JTy->getBitWidth());

      // store as Value*, not ConstantInt*, to avoid the compile error
      Value *ceV = ConstantInt::get(JTy, ceAP);

      // j_next = j_new + (c * e), in latch
      IRBuilder<> BL(Latch->getTerminator());
      Value *jNext = BL.CreateAdd(JNew, ceV, JOld->getName() + ".next");
      errs() << "[IVE-DEBUG]   Inserted latch update j_next = j_new + (c * e).\n";

      // Wire incoming edges
      JNew->addIncoming(j0, Preheader);
      JNew->addIncoming(jNext, Latch);
      errs() << "[IVE-DEBUG]   Wired PHI incomings from preheader/latch.\n";

      // Replace in-loop uses of old PHI with the new PHI
      int Replaced = 0;
      for (Use &U : llvm::make_early_inc_range(JOld->uses())) {
        if (auto *UserI = dyn_cast<Instruction>(U.getUser())) {
          if (UserI == JNew) continue;
          if (L.contains(UserI) && !isa<PHINode>(UserI)) {
            U.set(JNew);
            ++Replaced;
          }
        }
      }
      errs() << "[IVE-DEBUG]   Replaced " << Replaced
            << " loop-internal use(s) of " << JOld->getName() << ".\n";

      if (JOld->use_empty()) {
        errs() << "[IVE-DEBUG]   Old derived PHI now dead; erasing.\n";
        JOld->eraseFromParent();
      } else {
        errs() << "[IVE-DEBUG]   Old derived PHI still has "
              << JOld->getNumUses()
              << " remaining use(s) (likely outside the loop).\n";
      }

      LoopChanged = true;
    }


    if (LoopChanged)
      errs() << "[IVE-DEBUG]   Finished loop header: ";
    else
      errs() << "[IVE-DEBUG]   No changes made in loop header: ";

    Header->printAsOperand(errs(), false);
    errs() << "\n";

    return LoopChanged;
  }
};

} // namespace

// Pass registration
llvm::PassPluginLibraryInfo getInductionVarEliminationPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION,
    "InductionVarElimination",
    LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name,
           FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "induction-var-elimination") {
            FPM.addPass(InductionVarElimination());
            return true;
          }
          return false;
        }
      );
    }
  };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInductionVarEliminationPluginInfo();
}
