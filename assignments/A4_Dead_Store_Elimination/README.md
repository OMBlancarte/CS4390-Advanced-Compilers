# MemorySSA Graph Visualization

MemorySSAGraphVisualization.cpp is a extended version of MemorySSADemo.cpp and generates a graphical representtation of LLVM's MemorySSA in DOT format, which can be visualized using Graphviz.

## Features
* Visual representation of MemorySSA structures
* Color-coded nodes(Help from AI):
  * Blue diamonds: MemoryPhi nodes (merge points)
  * Red boxes: MemoryDef nodes (stores/writes)
  * Green boxes: MemoryUse nodes (loads/reads)
* Edge types(Help from AI):
  * Solid red: Definition edges (what defines what)
  * Dashed green: Use edges (what reads depend on)
  * Blue: Phi incoming edges (control flow merges)
* Basic block clustering: Groups memory accesses by basic block
* Instruction details: Shows the actual LLVM IR instruction for each access

## Building the Pass
On macOS:
```
clang++ -std=c++17 -fPIC \
  -shared MemorySSAGraphVisualizer.cpp -o libMemorySSAGraphVis.dylib \
  $(llvm-config --cxxflags --ldflags) -lLLVM
```
On Linux:
```
clang++ -std=c++17 -fPIC \
  -shared MemorySSAGraphVisualizer.cpp -o libMemorySSAGraphVis.so \
  $(llvm-config --cxxflags --ldflags) -lLLVM
```

## Preparing Test Files
Step 1:Compile C to LLVM IR
```
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm <file>.c -o <file>.ll
```
Step 2: Simplify with mem2reg
```
opt -passes=mem2reg <file>.ll -S -o <file>_simplified.ll
```

## Running the Visualization Pass
On macOS:
```
opt -load-pass-plugin=./libMemorySSAGraphVis.dylib \
    -passes="memssa-graph-vis" \
    test_memssa_simplified.ll \
    -disable-output
```
On Linux:
```
opt -load-pass-plugin=./libMemorySSAGraphVis.so \
    -passes="memssa-graph-vis" \
    test_memssa_simplified.l
```
This creates a ```.dot``` file for each function in the input (e.g., ```simple_example_memssa.dot```).

Generating Visual Output
PDF:
```
dot -Tpdf simple_example_memssa.dot -o simple_example_memssa.pdf
```

# Dead Store Elimination (DSE)
A LLVM pass that implements intraprocedural dead store elimination using MemorySSA. There is also a test suite file named ```dse_test_suite.c``` which will be expained how to build and an overview of its contents.

## Key Components
1. Reverse Program Traversal
Processes stores from bottom to top to find stores that are immediately overwritten.
2. Clobbering Memory Access
Uses MemorySSA's walker to find the previous memory write that could affect the same location.
3. Must-Alias
Verifies that both stores write to the exact same memory location using LLVM's AliasAnalysis.
4. Intervening Use Check
Ensures no loads read from the dead store between the two stores.
5. Post Domination
Confirms that the later store always executes after the earlier one (every path from the earlier store reaches the later store).
6. MemorySSAUpdater
Maintains MemorySSA invariants after removing dead stores.

# Building the Pass
On macOS:
```
clang++ -std=c++17 -fPIC \
  -shared DeadStoreElimination.cpp -o libDeadStoreElimination.dylib \
  $(llvm-config --cxxflags --ldflags) -lLLVM
```
On Linux:
```
clang++ -std=c++17 -fPIC \
  -shared DeadStoreElimination.cpp -o libDeadStoreElimination.so \
  $(llvm-config --cxxflags --ldflags) -lLLVM
```

# Running the DSE Pass
Step 1: Compile test file to LLVM IR
```
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm dse_test_suite.c -o dse_test_suite.ll
```
Step 2: Run mem2reg to promote to SSA
```
opt -passes=mem2reg dse_test_suite.ll -S -o dse_test_suite_simplified.ll
```
Step 3: Run DSE pass (macOS)
```
opt -load-pass-plugin=./libDeadStoreElimination.dylib \
    -passes="dse" \
    dse_test_suite_simplified.ll \
    -S -o dse_test_suite_optimized.ll
```
Step 3: Run DSE pass (Linux)
```
opt -load-pass-plugin=./libDeadStoreElimination.so \
    -passes="dse" \
    dse_test_suite_simplified.ll \
    -S -o dse_test_suite_optimized.ll
```

# Test Suite Overview (AI helped generate test cases)
The test suite includees 20 comprehensive test cases covering:
✅ Cases WHERE Dead Stores SHOULD Be Eliminated
1. test1_simple_dead_store: Basic case - ```*p=1; *p=2;```
2. test2_multiple_dead_stores: Chain of stores - ```*p=1; *p=2; *p=3;```
3. test4_computed_dead_store: Dead computed value - ```*p=x*2; *p=x+5;```
4. test5_same_location_different_syntax: ```p[0]=100; *p=200;```
5. test7_unconditional_overwrite: All branches overwrite initial store
6. test11_array_dead_stores: ```arr[0]=1; arr[0]=2;```
7. test13_chain_of_dead_stores: Multiple stores in sequence
8. test14_pointer_arithmetic: ```*p=10; *(p+0)=20;```
9. test15_struct_members: ```pt->x=1; pt->x=2;```
10. test18_complex_postdom: All control flow paths overwrite
11. test20_known_indices: ```arr[5]=42; arr[5]=99;```

❌ Cases WHERE Dead Stores SHOULD NOT Be Eliminated
1. test3_store_with_use: Store is read before being overwritten
2. test6_conditional_not_dead: Only some paths overwrite
3. test8_different_locations: Different pointers may not alias
4. test9_loop_dead_store: Loop iterations need intermediate values
5. test10_dead_init_before_loop: Loop might not execute (n could be 0)
6. test12_early_return: Later store doesn't post-dominate (early return possible)
7. test16_call_may_use: Function call might read the value
8. test19_partial_loop: Initial value used if loop doesn't execute