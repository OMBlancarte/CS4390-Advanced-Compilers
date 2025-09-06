import json
import sys
from collections import OrderedDict, deque

TERMINATORS = 'jmp', 'call', 'ret'

def get_path_lengths(cfg, entry):
    """
    Compute the shortest path length (in edges) from the entry node to each node in the CFG.

    Parameters:
        cfg (dict): mapping {node: [successors]}
        entry (str): starting node

    Returns:
        dict: {node: distance from entry}, unreachable nodes are omitted
    """
    if entry not in cfg:
        return {}
    
    distances = {entry: 0}
    queue = deque([entry])

    while queue:
        current = queue.popleft()
        current_dist = distances[current]

        for succ in cfg[current]:
            if succ not in distances:
                distances[succ] = current_dist + 1
                queue.append(succ)
    
    return distances

def reverse_postorder(cfg, entry):
    """
    Compute reverse postorder (RPO) for a CFG.

    Parameters:
        cfg (dict): mapping {node: [successors]}
        entry (str): starting node

    Returns:
        list: nodes in reverse postorder
    """


def find_back_edges(cfg, entry):
    """
    Find back edges in a CFG using DFS.

    Parameters:
        cfg(dict): mapping {node: [successors]}
        entry(str): starting node

    Returns: 
        list of edges (u,v) where u->v is a back edge
    """

def is_reducible(cfg, entry):
    """
    Determine whether a CFG is reducible.

    Parameters:
        cfg(dict): mapping {node: [successors]}
        entry(str): starting nod

    Returns:
        True if the CFG is reducible or False if the CFG is irreducible
    """

def form_blocks(body):
    cur_block  = []

    for instr in body:
        if 'op' in instr:   # An actual instruction
            cur_block.append(instr)

            # Check for terminator
            if instr['op'] in TERMINATORS:
                yield cur_block
                cur_block = []

        else:   # A label
            if cur_block:
                yield cur_block

            cur_block = [instr]
    if cur_block:
        yield cur_block


def block_map(blocks):
    out = OrderedDict()

    for block in blocks:
        if 'label' in block[0]:
            name = block[0]['label']
            block = block[1:]
        else:
            name = 'b{}'.format(len(out))

        out[name] = block

    return out

def get_cfg(name2block):
    """Given a name-to-block map, produce a mapping from block names to
    successor block names
    """
    out = {}
    for i, (name, block) in enumerate(name2block.items()):
        last = block[-1]

        if last['op'] in ('jmp', 'br'):
            succ = last['labels']
        elif last['op'] == 'ret':
            succ = []
        else:
            if i == len(name2block) - 1:
                succ = []
            else:
                succ = [list(name2block.keys())[i + 1]]

        out[name] = succ
    return out

def mycfg():
    prog = json.load(sys.stdin)
    for func in prog['functions']:
        name2block = block_map(form_blocks(func['instrs']))
        # for name, block in name2block.items():
        #     print(name)
        #     print(' ', block)
        cfg = get_cfg(name2block)
        # print(cfg)

        entry = list(name2block.keys())[0]

        # Test the implemented functions
        print(f"Function: {func['name']}")
        print(f"Path lengths: {get_path_lengths(cfg, entry)}")

        # Original CFG visualization
        print('digraph {} {{'.format(func['name']))
        for name in name2block:
            print(' {};'.format(name))
        for name, succs in cfg.items():
            for succ in succs:
                print(' {} -> {};'.format(name, succ))
        print('}')


if __name__ == '__main__':
    mycfg()