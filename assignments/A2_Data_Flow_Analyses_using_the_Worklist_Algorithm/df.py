import sys
import json
from collections import namedtuple

from form_blocks import form_blocks
import cfg

# A single dataflow analysis consists of these part:
# - forward: True for forward, False for backward.
# - init: An initial value (bottom or top of the latice).
# - merge: Take a list of values and produce a single value.
# - transfer: The transfer function.
Analysis = namedtuple("Analysis", ["forward", "init", "merge", "transfer"])


def union(sets):
    out = set()
    for s in sets:
        out.update(s)
    return out


def df_worklist(blocks, analysis):
    """The worklist algorithm for iterating a data flow analysis to a
    fixed point.
    """
    preds, succs = cfg.edges(blocks)

    # Switch between directions.
    if analysis.forward:
        first_block = list(blocks.keys())[0]  # Entry.
        in_edges = preds
        out_edges = succs
    else:
        first_block = list(blocks.keys())[-1]  # Exit.
        in_edges = succs
        out_edges = preds

    # Initialize.
    in_ = {first_block: analysis.init}
    out = {node: analysis.init for node in blocks}

    # Iterate.
    worklist = list(blocks.keys())
    while worklist:
        node = worklist.pop(0)

        inval = analysis.merge(out[n] for n in in_edges[node])
        in_[node] = inval

        outval = analysis.transfer(blocks[node], inval)

        if outval != out[node]:
            out[node] = outval
            worklist += out_edges[node]

    if analysis.forward:
        return in_, out
    else:
        return out, in_


def fmt(val):
    """Guess a good way to format a data flow value. (Works for sets and
    dicts, at least.)
    """
    if isinstance(val, set):
        if val:
            return ", ".join(v for v in sorted(val))
        else:
            return "∅"
    elif isinstance(val, dict):
        if val:
            return ", ".join("{}: {}".format(k, v) for k, v in sorted(val.items()))
        else:
            return "∅"
    else:
        return str(val)


def run_df(bril, analysis_name):
    for func in bril["functions"]:
        # Form the CFG.
        blocks = cfg.block_map(form_blocks(func["instrs"]))
        cfg.add_terminators(blocks)

        # Reaching definitions and available expressions analysis
        if analysis_name == "rdefs":
            analysis = create_reaching_defs_analysis(blocks)
        else:
            analysis = ANALYSES[analysis_name]

        in_, out = df_worklist(blocks, analysis)
        for block in blocks:
            print("{}:".format(block))
            print("  in: ", fmt(in_[block]))
            print("  out:", fmt(out[block]))


def gen(block):
    """Variables that are written in the block."""
    return {i["dest"] for i in block if "dest" in i}


def use(block):
    """Variables that are read before they are written in the block."""
    defined = set()  # Locally defined.
    used = set()
    for i in block:
        used.update(v for v in i.get("args", []) if v not in defined)
        if "dest" in i:
            defined.add(i["dest"])
    return used


def cprop_transfer(block, in_vals):
    out_vals = dict(in_vals)
    for instr in block:
        if "dest" in instr:
            if instr["op"] == "const":
                out_vals[instr["dest"]] = instr["value"]
            else:
                out_vals[instr["dest"]] = "?"
    return out_vals


def cprop_merge(vals_list):
    out_vals = {}
    for vals in vals_list:
        for name, val in vals.items():
            if val == "?":
                out_vals[name] = "?"
            else:
                if name in out_vals:
                    if out_vals[name] != val:
                        out_vals[name] = "?"
                else:
                    out_vals[name] = val
    return out_vals


def collect_all_defs_by_var(blocks):
    """
    Gather every definition site in the function, grouped by variable.
    Each def site is identified as 'var@blockName.idx'.
    """
    defs_by_var = {}
    for bname, block in blocks.items():
        for idx, instr in enumerate(block):
            if "dest" in instr:
                var = instr["dest"]
                def_id = f"{var}@{bname}.{idx}"
                defs_by_var.setdefault(var, set()).add(def_id)
    return defs_by_var

def gen_last_defs_for_block(bname, block):
    """
    GEN[b]: the set of definitions in block b that reach the END of b.
    That is, the **last** definition per variable in this block.
    """
    seen = set()         # variables we've already recorded (walking backwards)
    gen_b = set()
    for idx in range(len(block) - 1, -1, -1):
        instr = block[idx]
        if "dest" in instr:
            v = instr["dest"]
            if v not in seen:
                seen.add(v)
                gen_b.add(f"{v}@{bname}.{idx}")
    return gen_b

def kill_for_block(gen_b, defs_by_var):
    """
    KILL[b]: for variables defined in this block, kill **all other defs**
    of those variables (i.e., every def site except the ones in GEN[b]).
    """
    vars_defined = {d.split("@")[0] for d in gen_b}
    kill_b = set()
    for v in vars_defined:
        kill_b |= (defs_by_var.get(v, set()) - {d for d in gen_b if d.startswith(v + "@")})
    return kill_b

def build_rdefs(blocks):
    """
    Build per-block GEN/KILL maps (keyed by id(block)) for reaching definitions.
    Uses last-def-per-var GEN so GEN truly represents what reaches block end.
    """
    defs_by_var = collect_all_defs_by_var(blocks)

    gen_map = {}
    kill_map = {}

    for bname, block in blocks.items():
        bid = id(block)
        gen_b  = gen_last_defs_for_block(bname, block)
        kill_b = kill_for_block(gen_b, defs_by_var)

        gen_map[bid]  = gen_b
        kill_map[bid] = kill_b

    return gen_map, kill_map

def create_reaching_defs_analysis(blocks):
    """Create reaching definitions analysis"""
    gen_map, kill_map = build_rdefs(blocks)

    return Analysis(
        forward=True,
        init=set(),
        merge=union,
        transfer=lambda block, in_defs: (in_defs - kill_map[id(block)]) | gen_map[id(block)]
    )

ANALYSES = {
    # A really really basic analysis that just accumulates all the
    # currently-defined variables.
    "defined": Analysis(
        True,
        init=set(),
        merge=union,
        transfer=lambda block, in_: in_.union(gen(block)),
    ),
    # Live variable analysis: the variables that are both defined at a
    # given point and might be read along some path in the future.
    "live": Analysis(
        False,
        init=set(),
        merge=union,
        transfer=lambda block, out: use(block).union(out - gen(block)),
    ),
    # A simple constant propagation pass.
    "cprop": Analysis(
        True,
        init={},
        merge=cprop_merge,
        transfer=cprop_transfer,
    ),
}

if __name__ == "__main__":
    bril = json.load(sys.stdin)
    run_df(bril, sys.argv[1])
