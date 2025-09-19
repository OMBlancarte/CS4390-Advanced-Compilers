# Assignment 1: Working with CFGs
This assignment extends the CFG utilities from A0. This repo contains a script that:

1. Parses Bril functions into basic blocks
2. Builds a CFG (```get_cfg```)
3. Implements and demonstrates:
   * ```get_path_lengths(cfg, entry)```
   * ```reverse_postorder(cfg, entry)```
   * ```find_back_edges(cfg, entry)```
   * ```is_reducible(cfg, entry)```
4. Prints a Graphviz digraph you can pipe to dot for visualization but you need to comment out the analysis print lines on lines 226 to 231.

## How it works
* ```form_blocks(body)``` : splits a Bril instruction list into basic blocks using labels and terminators (```jmp```, ```call```, ```ret```).
* ```block_map(blocks)``` : maps block names → block instruction lists.
* ```get_cfg(name2block)``` : builds a dict ```{block: [successors]}``` using the last instruction of each block (```jmp```, ```call```, ```ret```, or fallthrough).
* ```get_path_lengths(cfg, entry)``` : BFS shortest edge distances from ```entry```.
* ```reverse_postorder(cfg, entry)``` : DFS postorder, then reverses the list for RPO.
* ```find_back_edges(cfg, entry)``` : DFS with colors (WHITE/GRAY/BLACK) records edges to ancestors (GRAY). 
* ```is_reducible(cfg, entry)``` : iterative dominators over the reachable subgraph; checks every back edge ```(u,v)``` has ```v``` dominating ```u```.
The ```mycfg()``` entrypoint:
* Reads a Bril program (JSON) from stdin.
* For each function:
  * Prints path lengths, RPO, backedges, reducibiliity.
  * Emits a Graphviz digraph of the CFG

## Usage
I believe it is easier to run from the A1_Working_with_CFGs directory: 
```
# Print analysis + Graphviz to stdout
bril2json < test/irreducible.bril | python3 myCFG.py

# Render a visual CFG from Graphviz to a pdf
bril2json < test/irreducible.bril 
    | python3 myCFG.py
    | dot -Tpdf -o cfg.pdf
```
> [!IMPORTANT]
> If you try rendering a visual CFG without commenting out the following printing analysis lines then you will get an error.

```
print(f"Function: {func['name']}")
print(f"Path lengths: {get_path_lengths(cfg, entry)}")
print(f"Reverse postorder: {reverse_postorder(cfg, entry)}")
print(f"Back edges: {find_back_edges(cfg, entry)}")
print(f"Is reducible: {is_reducible(cfg, entry)}")
print()
```
