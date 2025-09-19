# Assignment 1: Working with CFGs
This assignment extends the CFG utilities from A0. This repo contains a script that:

1. Parses Bril functions into basic blocks
2. Builds a CFG (```get_cfg```)
3. Implements and demonstrates:
   * ```get_path_lengths(cfg, entry)```
   * ```reverse_postorder(cfg, entry)```
   * ```find_back_edges(cfg, entry)```
   * ```is_reducible(cfg, entry)```
4. Prints a Graphviz digraph you can pipe to dot for visualization.

## How it works
* ```form_blocks(body)``` : splits a Bril instruction list into basic blocks using labels and terminators (```jmp```, ```call```, ```ret```).