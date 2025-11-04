# LLVM Assignment 1 – Loop Optimization Passes (LICM & IVE)

## 1. Overview
This project adds three LLVM passes to the llvm-tutor framework:
* SimpleLICM – a loop-invariant code motion pass (worklist-based).
* ExtendedDerivedIV – an analysis pass that reports (derived) induction variables across nested loops using ScalarEvolution.
* InductionVarElimination – a transformation that rewrites derived IVs into canonical PHI recurrence based on a basic IV. Prints rich ```[IVE-DEBUG]``` logs.
You will build these passes as llvm-tutor plugins, run them on sample inputs (e.g., matmul_canonical.ll), and verify that optimized IR preserves program behavior.

## 2. Repository layout
Suggested structure inside your fork/clone of llvm-tutor:
```
llvm-tutor/
  lib/
    SimpleLICM.cpp                 # this repo
    ExtendedDerivedIV.cpp          # this repo
    InductionVarElimination.cpp    # this repo
    CMakeLists.txt                 # add targets + pipeline registration
  inputs/
    matmul.c
    loops.c
    iveTest.c
  build/
  outputs/
    matmul_canonical.ll           
```
Source files in this submission:
SimpleLICM.cpp (implementation & registration) 
ExtendedDerivedIV.cpp (nested-loop analysis) 
InductionVarElimination.cpp (derived-IV elimination)

## 3. Build instructions (LLVM 21 + llvm-tutor)
1. Configure and build (from an out-of-source build directory):
```
export LLVM_DIR="$(llvm-config-21 --prefix)"
echo "$LLVM_DIR"    # Sanity check
mkdir -p build && cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make
```
If CMake can see LLVM via ```LLVM_DIR``` or ```LT_LLVM_INSTALL_DIR```, it will emit “Found LLVM … Using LLVMConfig.cmake …” during configuration (typical with llvm-tutor).

Make sure your ```lib/CMakeLists.txt``` registers targets and maps pass names:

* Pipeline names used at the command line:
  * simple-licm (loop pass)
  * extended-derived-iv (function pass) 
  * induction-var-elimination (function pass)

## 4. How to run the passes
All commands below are run from ```build/```. Replace library names if your platform uses ```.dylib```, ```.so```, or ```.dll```.

### A. SimpleLICM (worklist LICM)
Run on canonicalized matrix-multiply IR (or any looped IR):
```
opt -load-pass-plugin ./lib/libSimpleLICM.* \
    -passes='loop(simple-licm)' \
    -S -o ../outputs/matmul_licm.ll ../outputs/matmul_canonical.ll
```
The pass hoists register-to-register, memory-free, non-PHI instructions that dominate all loop exits into the preheader and emits lines like:
```
Hoisting:   %7 = sext i32 %.037 to i64
```
Behavior and safety checks (dominance of exits, speculative-safety) are implemented in SimpleLICM.cpp.
Assignment requirements for LICM appear in the brief.
### B. ExtendedDerivedIV (nested-loop IV analysis)
```
opt -load-pass-plugin ./lib/libExtendedDerivedIV.* \
    -passes='extended-derived-iv' \
    -disable-output ../outputs/matmul_canonical.ll
```
Sample output structure:
```
Loop at depth 1 (header: for.body):
  Derived induction variable: j = {Start,+,Step}<for.body>
  ...
```
Implementation detects affine ```SCEVAddRecExpr``` PHIs and prints ```{Start,+,Step}``` at each depth. 

### C. InductionVarElimination
```
opt -load-pass-plugin ./lib/libInductionVarElimination.* \
    -passes='induction-var-elimination' \
    -S -o ../outputs/iveTest_IVE.ll ../outputs/iveTest_canonical.ll
```
What it does:
1. Finds a basic IV ```i``` with constant stride ```e``` in the header PHIs.
2. For each derived IV ```j = c*i + d```, creates:
   * Preheader init ```j0 = d```
   * Latch update ```j_next = j + (c*e)```
   * Header PHI ```j = phi [j0, preheader], [j_next, latch]```
3. Replaces in-loop uses of old j and removes it if dead; logs steps with ```[IVE-DEBUG]```.