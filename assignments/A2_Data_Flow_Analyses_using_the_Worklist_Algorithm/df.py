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

    # Direction setup
    if analysis.forward:
        first_block = list(blocks.keys())[0]  # Entry.
        in_edges = preds
        out_edges = succs
    else:
        first_block = list(blocks.keys())[-1]  # Exit.
        in_edges = succs
        out_edges = preds

    # compute reachable nodes in the chosen direction 
    from collections import deque

    reachable = set()
    dq = deque([first_block])
    while dq:
        n = dq.popleft()
        if n in reachable:
            continue
        reachable.add(n)
        for m in out_edges[n]:
            if m not in reachable:
                dq.append(m)

    # Initialize only for reachable nodes
    in_ = {first_block: analysis.init}
    out = {node: analysis.init for node in reachable}

    # Worklist over reachable nodes only
    worklist = list(reachable)
    while worklist:
        node = worklist.pop(0)

        # Merge only from reachable predecessors
        inval = analysis.merge(out[n] for n in in_edges[node] if n in reachable)
        in_[node] = inval

        outval = analysis.transfer(blocks[node], inval)

        if outval != out[node]:
            out[node] = outval
            # Enqueue only reachable successors
            for m in out_edges[node]:
                if m in reachable:
                    worklist.append(m)

    # Return in/out only for reachable nodes
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
        elif analysis_name == "available":
            analysis = create_available_exps_analysis(blocks)
        else:
            analysis = ANALYSES[analysis_name]

        in_, out = df_worklist(blocks, analysis)

        # Union of keys just in case; out usually suffices
        reachable_keys = set(out.keys()) | set(in_.keys())

        for block in blocks:
            print(f"{block}:")
            if block not in reachable_keys:
                # mark unreachable
                print("  in:  ∅  (unreachable)")
                print("  out: ∅  (unreachable)")
                continue
            print("  in: ", fmt(in_.get(block, analysis.init)))
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
    GEN[b]: the set of last defs per var in block.
    Scan backwards so only final def of eeach var is kept.
    """
    seen = set()         # variables already seen
    gen_b = set()
    for idx in range(len(block) - 1, -1, -1):   # backward scan
        instr = block[idx]
        if "dest" in instr:
            v = instr["dest"]
            if v not in seen:   # keep only the last def of v
                seen.add(v)
                gen_b.add(f"{v}@{bname}.{idx}")
    return gen_b

def kill_for_block(gen_b, defs_by_var):
    """
    KILL[b]: all other defs of the vars defined in this block
    For each var in GEN[b], kill every other def of that var in the function.
    """
    # Extract vars defined in the blocks GEN set
    vars_defined = {d.split("@")[0] for d in gen_b}
    kill_b = set()
    for v in vars_defined:
        # All defs of var v minus the ones in GEN[b]
        kill_b |= (defs_by_var.get(v, set()) - {d for d in gen_b if d.startswith(v + "@")})
    return kill_b

def build_rdefs(blocks):
    """
    Build GEN/KILL maps for each block, keyed by id(block).
    - GEN[b] = last defs per var in b
    - KILL[b] = all other defs of those vars
    """
    # Map each var to def in function
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

# Operations that are communtative
COMMUTATIVE_OPS = {"add", "mul", "and", "or", "ep" }

def expr_key(instr):
    """Convert an instruction into a canonical expression string"""
    op = instr.get("op")
    if not op or op == "const" or op == "print" or op == "br":     # skip constants and non-exprs
        return None
    args = instr.get("args", [])
    if not args:
        return None
    # For communtative ops, sort arguments to match (e.g. "a+b" and "b+a" match)
    if op in COMMUTATIVE_OPS:
        key_args = ",".join(sorted(args))
    else:
        key_args = ",".join(args)
    return f"{op}({key_args})"

def collect_all_expressions(blocks):
    """Universe U = all expressions in the function."""
    universe = set()
    # Scan every instruction in every block
    for block in blocks.values():
        for instr in block:
            expr = expr_key(instr)
            if expr:                 # Only expressions count (skip const, jmp, ret, etc.)
                universe.add(expr)
    return universe


def gen_expressions_for_block(block):
    """
    GEN[b] = expressions computed in b that are available at exit.
    We scan backwards so that later redefinitions kill earlier exprs.
    """
    gen_b = set()
    defs_after = set()  # variables defined later in the block
    for instr in reversed(block):
        if "dest" in instr:
            defs_after.add(instr["dest"])  # this var is defined later
        k = expr_key(instr)
        if k:
            # Extract operands from expr string "op(a,b,...)"
            start = k.find("(") + 1
            end = k.rfind(")")
            vars_in_expr = set(k[start:end].split(",")) if end > start else set()
            # Only include if none of its operands are redefined later
            if not (vars_in_expr & defs_after):
                gen_b.add(k)
    return gen_b


def kill_expressions_for_block(block, universe):
    """KILL[b] = all exprs in U that mention a var defined in b."""
    defs = {instr["dest"] for instr in block if "dest" in instr}
    killed = set()
    if defs:
        for expr in universe:
            # Pull out variables used in this expression
            inside = expr[expr.find("(")+1:expr.rfind(")")]
            vars_in_expr = set(inside.split(",")) if inside else set()
            # If this block defines any of them, the expr is killed
            if vars_in_expr & defs:
                killed.add(expr)
    return killed
    

def build_available_expressions(blocks):
    """Build GEN/KILL maps and universe for available expressions"""
    u = collect_all_expressions(blocks)
    gen_map = {}
    kill_map = {}
    for bname, block in blocks.items():
        bid = id(block)
        gen_map[bid] = gen_expressions_for_block(block)
        kill_map[bid] = kill_expressions_for_block(block, u)
    return u, gen_map, kill_map

def merge_intersection(u):
    """
    Meet operator for AE:
    - If no preds (entry), assume Universe (all exprs available).
    - Otherwise intersect incoming sets.
    """
    def intersection(iter_vals):
        vals = list(iter_vals)
        if not vals:
            return set(u)   # If no entry assume all
        result = set(u)
        for v in vals:
            result &= v     # Intersect across predecessors
        return result
    return intersection

def create_available_exps_analysis(blocks):
    """Construct available expressions analysis"""
    universe, gen_map, kill_map = build_available_expressions(blocks)
    merge_fn = merge_intersection(universe)

    return Analysis(
        forward=True,
        init=set(universe),
        merge=merge_fn,
        transfer=lambda block, in_exprs:
            # OUT[b] = (IN[b] − KILL[b]) ∪ GEN[b]
            (in_exprs - kill_map[id(block)]) | gen_map[id(block)]
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
