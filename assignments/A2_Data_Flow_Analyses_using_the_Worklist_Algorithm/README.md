# Assignment 2: Dataflow Analyses using the Worklist Algorithm
This assignment extends the dataflow analysis framework (```df.py```) to implement:
* **Task 3**: Reaching Defintions
* **Task 4**: Available Expressions
The repo contains code that:
* Parses Bril functions into basic blocks
* Builds a CFG using ```form_blocks``` and ```cfg``` utilities
* Runs dataflow analyses using a generic worklist solver (```df_worklist```)
* Implements and demonstrates:
  * **Reaching Definitions** analysis
  * **Available Expressions** analysis

## Task 3: Reaching Definitions
### What it does
Reaching Definitions determines which assignments to variables (definitions) may reach a program point without being overwritten.
* **GEN[b]**: the set of last definitions per variable in block ```b```.
* **KILL[b]**: ll other definitions of variables defined in block ```b```.
### How it works
* ```collect_all_defs_by_var(blocks)```: Collects all definition sites across the function.
* ```gen_last_defs_for_block(bname, block)```: Computes the last definitions per variable in the block.
* ```kill_for_block(gen_b, defs_by_var)```: Finds all other definitions of the same variables.
* ```build_rdefs(blocks)```: Builds GEN/KILL sets per block.
* ```create_reaching_defs_analysis(blocks)```: Creates an Analysis object with forward direction, union merge, and the RD transfer function.
### Usage
```
# Run Reaching Definitions on a Bril program
bril2json < test/prog.bril | python3 df.py rdefs
```

## Task 4: Available Expressions
### What it does
Available Expressions identifies expressions that are guaranteed to have been computed on all paths reaching a program point, and whose operands are not redefined afterwards.
* **GEN[b]**: expressions evaluated in ```b``` that are still valid at exit (no later redefinitions).
* **KILL[b]**: all expressions in the universe that use variables redefined in ```b```.
### How it works
* ```expr_key(instr)```: Converts instructions into canonical expression strings (```op(a,b)```), sorting operands for commutative ops.
* ```collect_all_expressions(blocks)```: Builds the universe of expressions.
* ```gen_expressions_for_block(block)```: Scans block backward; includes exprs only if operands are not redefined later.
* ```kill_expressions_for_block(block, universe)```: Marks all expressions mentioning vars redefined in ```b```.
* ```build_available_expressions(blocks)```: Builds GEN/KILL maps and the universe.
* ```create_available_expressions_analysis(blocks)```: Creates an Analysis with forward direction, intersection merge, and the AE transfer function.
### Usage
```
# Run Available Expressions on a Bril program
bril2json < test/prog.bril | python3 df.py available
```

## Output Formate
The solver prints ```in``` and ```out``` sets for each block in the function:
```
b1:
  in:  ∅
  out: v@b1.0
end:
  in:  v@b1.0
  out: v@b1.0
```
* **Reaching Definitions**:  elements are definition sites ```var@block.index```.
* **Available Expressions**: elements are canonical expressions ```op(arg1,arg2)```.