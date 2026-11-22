#!/usr/bin/env python3
"""Tests of cfgformat.py: each case is a configuration, and the checks on
what the formatter makes of it. Run: python3 test_cfgformat.py"""

import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cfgformat as cf  # noqa: E402

RULE = cf.RULE


def titled(t):
    """A line of the rule with t in it, starting on an odd column, i.e.,
    with two spaces before it if its length is even."""
    s = (cf.WIDTH - len(t)) // 2
    s += 1 - s % 2
    line = list(RULE)
    line[s - 1] = ' '
    line[s:s + len(t)] = list(t)
    if s + len(t) < cf.WIDTH:
        line[s + len(t)] = ' '
    return ''.join(line)


def boxed(name, body, title=None, end=None):
    """A configuration in a box, with the titles given."""
    title = title or titled(name)
    end = end or titled('END ' + name)
    return '\n'.join([RULE, title, '#', '# what it is', '#', RULE, RULE,
                      '', body, '', RULE, end, RULE]) + '\n'


EMPTY = """ComputeConfig # exact type of the ComputeConfig object
1 # f_diff
0 # number of integer parameters
0 # number of double parameters
0 # number of string parameters
0 # number of vector-of-int parameters
0 # number of vector-of-double parameters
0 # number of vector-of-string parameters
* # [none]"""

PARS = """ComputeConfig # exact type of the ComputeConfig object
0 # f_diff
3 # number of integer parameters
intMaxIter    100  # the iterations
intLogVerb 0     # the log
intMinIter    3  # at least these
1 # number of double parameters
dblRelAcc 1e-6 # the relative accuracy
0 # number of string parameters
0 # number of vector-of-int parameters
0 # number of vector-of-double parameters
0 # number of vector-of-string parameters
* # [none]"""

BSC = """BlockSolverConfig # exact type
1 # differential
2 # number of Solver
MILPSolver
BundleSolver # the dual
2 # number of ComputeConfig
*MILPCfg.txt
*BaseCfg.txt +
1
1
intMaxIter 5
0
0
0
0
0
*"""

CASES = [
    # name, file name, text, lines that the result must contain, and must
    # not, None if it must be left as it is
    ('an empty ComputeConfig gets its structural comments', 'ECfg.txt',
     boxed('ECfg.txt', EMPTY),
     ['# now all the integer parameters', '# [none]', '*  # [none]',
      '1  # f_diff'], []),
    ('a table in the columns that most of its rows use', 'PCfg.txt',
     boxed('PCfg.txt', PARS),
     ['intMaxIter    100  # the iterations',
      'intLogVerb    0    # the log', 'intMinIter    3    # at least these',
      'dblRelAcc 1e-6 # the relative accuracy'], []),
    ('a BlockSolverConfig with an override keeps the +', 'BSCfg.txt',
     boxed('BSCfg.txt', BSC),
     ['*BaseCfg.txt +', 'intMaxIter 5', 'BundleSolver # the dual'], []),
    ('a title of even length keeps its two spaces before', 'ABCDCfg.txt',
     boxed('ABCDCfg.txt', EMPTY), [], []),
    ('a title with the wrong name is written anew', 'RightCfg.txt',
     boxed('RightCfg.txt', EMPTY,
           title='# - - - - - - - - - - - - - - WrongCfg.txt - - - - - - -'),
     ['RightCfg.txt'], ['WrongCfg.txt']),
    ('a file with no Configuration in it is skipped', 'data.txt',
     boxed('data.txt', '3 4 5'), None, None),
    ('a file with no box is skipped', 'Plain.txt', EMPTY, None, None),
    ('tokens after the Configuration leave it as it is', 'TCfg.txt',
     boxed('TCfg.txt', EMPTY + '\nintStray 3'), None, None),
]


def run():
    work = tempfile.mkdtemp()
    failed = 0
    try:
        for name, fname, text, want, avoid in CASES:
            path = os.path.join(work, fname)
            with open(path, 'w') as f:
                f.write(text)
            base = os.path.join(work, 'BaseCfg.txt')   # of the overrides
            with open(base, 'w') as f:
                f.write(boxed('BaseCfg.txt', EMPTY))
            index = cf.build_index([path, base])
            bad = []
            try:
                out = cf.format_config(path, text, index)
            except (cf.Skip, cf.Fail):
                out = None
            if want is None:
                if out is not None and out != text:
                    bad.append('changed')
            elif out is None:
                bad.append('not formatted')
            else:
                bad += [w for w in want if w not in out]
                bad += [a for a in avoid if a in out]
                if name.startswith('a title of even length') and \
                   out.split('\n')[1] != text.split('\n')[1]:
                    bad.append('title moved')
                if cf.code_tokens(out) != cf.code_tokens(text):
                    bad.append('tokens')
                if any(len(l) > cf.WIDTH for l in out.split('\n')):
                    bad.append('past %d columns' % cf.WIDTH)
                if cf.format_config(path, out, index) != out:
                    bad.append('not idempotent')
            if bad:
                failed += 1
                print('FAIL %s: %s\n%s' % (name, bad, out))
            else:
                print('ok   %s' % name)
            os.remove(path)
            os.remove(base)
    finally:
        os.rmdir(work)
    print('%d of %d failed' % (failed, len(CASES)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(run())
