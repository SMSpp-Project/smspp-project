#!/usr/bin/env python3
"""Formats the .txt configuration files of SMS++.

A configuration is read with the grammar of the load() methods of the core
(Configuration::deserialize(), ComputeConfig, BlockConfig and OBlockConfig,
BlockSolverConfig, SimpleConfiguration< T > for T made of int, double,
std::string, Configuration *, std::pair, std::vector, std::list and
std::map), and written again in the layout of the configurations of SMS++:

  - the box of the head and the one of the end, 77 columns wide, with the
    name of the file and END <name>, and the rules of dashes in the body,
    such as # end of ComputeConfig - - -, on the columns of the box;
  - every element on a line of its own, an empty line between elements;
  - a number, or a *, and its comment two spaces apart, the type of a
    Configuration and its comment one space apart;
  - in the tables of the parameters and of the maps the values and the
    comments in a column each, a comment that does not fit continued on
    the lines below, aligned with it;
  - the structural comments of a ComputeConfig written as everywhere
    (# number of integer parameters, # now all the integer parameters,
    # [none] for a table that is empty, # pointer to the "extra"
    Configuration);
  - no line past 77 columns, no space at the end of a line.

Every other comment is kept: the comment lines right below an element (no
empty line between) belong to it, as the continuation of its comment or
as alternatives commented out, the ones after an empty line to the
element that follows them. Nothing that SMS++ reads changes: the sequence
of the tokens out of the comments is the same before and after, which the
script checks. A file that it cannot read with this grammar (e.g., a
Configuration of a module, which the core does not know) is left as it
is, and reported; a file whose first token is no Configuration (e.g., an
instance) is skipped.

Usage: cfgformat.py [--check | --diff] FILE...
With --check nothing is written: the files that would change are listed,
with exit status 1 if there are any; with --diff the changes are printed
instead of being written.
"""

import os
import re
import subprocess
import sys

WIDTH = 77
# the column of the comment on the line of the type, as in most of the
# configurations of SMS++; for any other type the spacing written is kept
TYPE_COL = {'BlockSolverConfig': 22, 'ComputeConfig': 14,
            'BlockConfig': 12, 'OBlockConfig': 14}
RULE = '# ' + '- ' * 37 + '-'
assert len(RULE) == WIDTH

STRUCTURAL = re.compile(r'#\s*(now all the .*parameters|\[none\]|'
                        r'pointer to the "extra" Configuration|'
                        r'number of .*parameters)\s*$')


class Fail(Exception):
    pass


class Skip(Exception):
    pass


# --------------------------------------------------------------------------
# reading


class Item:
    """An element: some tokens, on one line or on consecutive ones, with
    the comment lines before it and the ones right below it."""

    def __init__(self, toks, lines):
        self.toks = toks        # the tokens
        self.lines = lines      # the indices of the lines they are on
        self.comment = None     # the comment at the end of its last line
        self.before = []        # the comment lines before it
        self.after = []         # the comment lines right below it
        self.raw = []           # its lines as written, if more than one
        self.orig = ''          # its first line as written
        self.gap = False        # an empty line parts it, or the comments
                                # before it, from the element before

    def text(self):
        return ' '.join(self.toks)


class Reader:
    def __init__(self, text):
        self.lines = text.split('\n')
        self.toks = []
        for k, line in enumerate(self.lines):
            for t in line.split('#', 1)[0].split():
                self.toks.append((t, k))
        self.pos = 0
        self.owned = set()   # the lines given to an element

    def eof(self):
        return self.pos >= len(self.toks)

    def peek(self, d=0):
        p = self.pos + d
        return self.toks[p][0] if p < len(self.toks) else None

    def line_of(self, d=0):
        p = self.pos + d
        return self.toks[p][1] if p < len(self.toks) else None

    def take(self, n, what, multiline=False):
        if self.pos + n > len(self.toks):
            raise Fail('the file ends where %s is expected' % what)
        chunk = self.toks[self.pos:self.pos + n]
        lines = sorted(set(l for _, l in chunk))
        if len(lines) > 1 and not multiline:
            raise Fail('%s is split across lines (line %d)'
                       % (what, lines[0] + 1))
        self.pos += n
        it = Item([t for t, _ in chunk], lines)
        self.attach(it)
        return it

    def attach(self, it):
        first, last = it.lines[0], it.lines[-1]
        # the comment lines before it, back to the element before
        k = first - 1
        before = []
        while k >= 0 and k not in self.owned:
            s = self.lines[k].strip()
            if s.startswith('#'):
                before.insert(0, self.lines[k].rstrip())
            elif s:
                break
            else:
                before.insert(0, '')
            k -= 1
        for j in range(k + 1, first):
            self.owned.add(j)
        # the empty lines between the comments and the element are kept as
        # written, the ones at the head dropped, and remembered
        while before and before[0] == '':
            before.pop(0)
            it.gap = True
        if k >= 0 and not self.lines[k].strip():
            it.gap = True
        it.before = before
        for j in it.lines:
            self.owned.add(j)
        it.orig = self.lines[first].expandtabs(8)
        raw = self.lines[last]
        if '#' in raw:
            it.comment = raw.split('#', 1)[1].strip()
        if len(it.lines) > 1:
            it.raw = [self.lines[j].rstrip()
                      for j in range(first, last + 1)]
            for j in range(first, last + 1):
                self.owned.add(j)
        # the comment lines right below it
        k = last + 1
        while k < len(self.lines) and self.lines[k].strip().startswith('#') \
              and k not in self.owned:
            if self.next_code_line() is not None and \
               k >= self.next_code_line():
                break
            it.after.append(self.lines[k].rstrip())
            self.owned.add(k)
            k += 1

    def next_code_line(self):
        return self.toks[self.pos][1] if not self.eof() else None

    def tail(self):
        """The comment lines after the last element."""
        return [self.lines[k].rstrip() for k in range(len(self.lines))
                if k not in self.owned and self.lines[k].strip()]


# --------------------------------------------------------------------------
# the grammar


def is_int(t):
    return t is not None and re.match(r'[-+]?\d+$', t) is not None


class Parser:
    def __init__(self, reader, path, index):
        self.r = reader
        self.path = path
        self.index = index   # basename -> paths of the configurations
        self.out = []

    def emit(self, *e):
        self.out.append(e)

    def conf(self):
        r = self.r
        t = r.peek()
        if t is None:
            return
        if t.startswith('*'):
            if t != '*' and r.peek(1) == '+':
                if r.line_of(1) != r.line_of():
                    raise Fail('the + of an override is not on the line of '
                               'its file')
                it = r.take(2, 'an override')
                self.emit('override', it)
                self.body(self.type_of(t[1:]), override=True)
                return
            self.emit('ref', r.take(1, 'a Configuration'))
            return
        if is_int(t):
            raise Fail('a number where a Configuration is expected')
        it = r.take(1, 'the type of a Configuration')
        t = it.toks[0]
        tight = t.startswith('SimpleConfiguration<') and \
            (is_scalar(t[20:-1]) or t[20:-1].startswith('std::pair<'))
        self.emit('type', it, tight)
        self.body(t)

    def type_of(self, name):
        """The type of the Configuration in the file name, which an
        override is read as: looked for next to the file, and else by its
        name in the tree."""
        here = os.path.dirname(os.path.abspath(self.path))
        cands = [os.path.join(here, name)]
        cands += [p for p in self.index.get(os.path.basename(name), [])]
        types = set()
        for p in cands:
            if not os.path.exists(p):
                continue
            t = first_type(p, self.index, set())
            if t:
                types.add(t)
            if os.path.dirname(p) == here and t:
                return t
        if len(types) == 1:
            return types.pop()
        if not types:
            raise Fail('override of %s, which is not to be found' % name)
        raise Fail('override of %s, which is more files of more types'
                   % name)

    def body(self, cls, override=False):
        if cls == 'ComputeConfig':
            self.compute()
        elif re.match(r'(O?C?R?)BlockConfig$', cls):
            self.blockconfig(cls)
        elif cls == 'BlockSolverConfig':
            self.bsc()
        elif cls.startswith('SimpleConfiguration<') and cls.endswith('>'):
            self.value(cls[len('SimpleConfiguration<'):-1])
        else:
            raise Fail('a Configuration of type %s, which the script does '
                       'not know' % cls)

    KINDS = ['integer', 'double', 'string', 'vector-of-int',
             'vector-of-double', 'vector-of-string']

    def compute(self):
        r = self.r
        if not is_int(r.peek()):
            return
        self.emit('diff', r.take(1, 'the flags of a ComputeConfig'))
        for kind in self.KINDS:
            if not is_int(r.peek()):
                return
            cnt = r.take(1, 'the number of the %s parameters' % kind)
            n = int(cnt.toks[0])
            rows = []
            for i in range(n):
                if r.eof():
                    raise Fail('%d %s parameters declared, %d written'
                               % (n, kind, i))
                if kind.startswith('vector'):
                    m = 2 + vector_length(r, 1)   # name, count, values
                    rows.append(r.take(m, 'a parameter', multiline=True))
                else:
                    rows.append(r.take(2, 'a parameter'))
            self.emit('table', cnt, kind, rows)
        if not r.eof():
            self.emit('extra')
            self.conf()

    SLOTS = 10   # the Configurations of a BlockConfig, structure to extra

    def blockconfig(self, cls):
        """A BlockConfig, and the handlers its name says: O, the
        Configuration of the Objective; C, those of some Constraint; R,
        the BlockConfig of some sub-Block."""
        r = self.r
        if r.eof():
            return
        self.emit('number', r.take(1, 'the differential flag'))
        if r.eof():
            return
        self.emit('number', r.take(1, 'the version of the format'))
        for _ in range(self.SLOTS):
            if r.eof():
                return
            self.conf()
        handlers = cls[:-len('BlockConfig')]
        if 'O' in handlers and not r.eof():
            self.conf()
        if 'C' in handlers and not r.eof():
            cnt = r.take(1, 'the number of the Constraint')
            self.emit('number', cnt)
            for _ in range(int(cnt.toks[0])):
                self.emit('number', r.take(2, 'the index of a Constraint'))
                self.conf()
        if 'R' in handlers and not r.eof():
            cnt = r.take(1, 'the number of the sub-Block')
            self.emit('number', cnt)
            k = int(cnt.toks[0])
            for _ in range(abs(k)):
                if k < 0:
                    self.emit('number', r.take(1, 'the id of a sub-Block'))
                self.conf()

    def bsc(self):
        r = self.r
        if r.eof():
            return
        self.emit('number', r.take(1, 'the mode'))
        if r.eof():
            return
        cnt = r.take(1, 'the number of the Solver')
        self.emit('number', cnt)
        names = [r.take(1, 'the name of a Solver')
                 for _ in range(int(cnt.toks[0]))]
        self.emit('names', names)
        if r.eof():
            return
        cnt = r.take(1, 'the number of the ComputeConfig')
        self.emit('number', cnt)
        for _ in range(int(cnt.toks[0])):
            self.conf()

    def value(self, t):
        r = self.r
        t = t.strip()
        if is_conf(t):
            self.conf()
        elif t.startswith('std::pair<'):
            a, b = split_type(t[len('std::pair<'):-1])
            if is_scalar(a) and is_scalar(b):
                if r.line_of(1) == r.line_of():
                    self.emit('number', r.take(2, 'a pair'))
                else:
                    self.emit('number', r.take(1, 'a pair'))
                    self.emit('number', r.take(1, 'a pair'))
            elif is_scalar(a) and is_conf(b):
                key = r.take(1, 'the key of a pair')
                if r.peek() is None:
                    raise Fail('a key without its Configuration')
                if r.peek().startswith('*') and r.peek(1) != '+':
                    ref = r.take(1, 'the value of a pair')
                    self.emit('keyed', key, ref)
                else:
                    # a Configuration written out: the key on its line,
                    # and the Configuration below it
                    self.emit('number', key)
                    self.conf()
            else:
                self.value(a)
                self.value(b)
        elif re.match(r'std::(vector|list|map)<', t):
            if t.startswith('std::map<'):
                inner = 'std::pair<' + t[len('std::map<'):-1] + '>'
            else:
                inner = t[t.index('<') + 1:-1]
            if not is_int(r.peek()):
                raise Fail('a container without its number of elements')
            if is_scalar(inner):
                n = vector_length(r, 0)
                self.emit('raw', r.take(1 + n, 'a vector', multiline=True))
                return
            cnt = r.take(1, 'the number of the elements')
            n = int(cnt.toks[0])
            if n < 0:
                raise Fail('a sparse container of Configuration')
            self.emit('number', cnt)
            for _ in range(n):
                self.value(inner)
        elif is_scalar(t):
            self.emit('number', r.take(1, 'a value'))
        else:
            raise Fail('a value of type %s, which the script does not know'
                       % t)


def vector_length(r, d):
    """How many tokens a vector takes after its first one at peek(d), as
    operator>> reads it: k and k values, or -k, h and h pairs."""
    if not is_int(r.peek(d)):
        raise Fail('%s where the length of a vector is expected, most '
                   'likely a count that does not match the entries before'
                   % r.peek(d))
    k = int(r.peek(d))
    if k >= 0:
        return k
    if not is_int(r.peek(d + 1)):
        raise Fail('a sparse vector without its number of elements')
    return 1 + 2 * int(r.peek(d + 1))


def split_type(t):
    args, depth, cur = [], 0, ''
    for c in t:
        if c == '<':
            depth += 1
        elif c == '>':
            depth -= 1
        if c == ',' and depth == 0:
            args.append(cur.strip())
            cur = ''
        else:
            cur += c
    if cur.strip():
        args.append(cur.strip())
    return args


def is_scalar(t):
    return t.strip() in ('int', 'double', 'std::string', 'float', 'long',
                         'unsigned int', 'bool', 'char', 'unsigned long',
                         'long long', 'unsigned long long')


def is_conf(t):
    return t.replace(' ', '') == 'Configuration*'


def first_type(path, index, seen):
    """The type of the Configuration in the file path, following the
    files it includes."""
    if path in seen:
        return None
    seen.add(path)
    try:
        r = Reader(open(path, encoding='utf-8').read())
    except (OSError, UnicodeDecodeError):
        return None
    t = r.peek()
    if t is None or is_int(t):
        return None
    if t.startswith('*') and len(t) > 1:
        here = os.path.join(os.path.dirname(path), t[1:])
        if os.path.exists(here):
            return first_type(here, index, seen)
        for p in index.get(os.path.basename(t[1:]), []):
            x = first_type(p, index, seen)
            if x:
                return x
        return None
    return t


# --------------------------------------------------------------------------
# writing


def rule_line(line):
    if not line.strip():
        return ''
    """A comment of the body that ends with a rule of dashes, such as
    # end of ComputeConfig - - -, with its dashes on the columns of the
    box and up to its end; any other line as it is."""
    m = re.match(r'#\s*(.*?)\s*-?(\s*-){3,}\s*$', line.strip())
    if not m or not line.startswith('#'):
        return line
    text = m.group(1).strip()
    if not text:
        return RULE
    head = '# ' + text
    if len(head) + 2 > WIDTH:
        return line.rstrip()
    # one space after the text, then dashes up to the width
    out = head
    while len(out) + 2 <= WIDTH:
        out += ' -'
    return out


def wrap_comment(prefix, comment, col):
    """The line prefix + comment at col, the words that do not fit on the
    lines below at the same column."""
    if comment is None:
        return [prefix]
    pad = max(col - len(prefix), 1) if prefix else 0
    start = len(prefix) + pad
    if start + 2 + len(comment) <= WIDTH:
        return [prefix + ' ' * pad + '# ' + comment]
    words = comment.split()
    lines, cur = [], ''
    for w in words:
        if cur and start + 2 + len(cur) + 1 + len(w) > WIDTH:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + ' ' + w) if cur else w
    lines.append(cur)
    out = [prefix + ' ' * pad + '# ' + lines[0]]
    out += [' ' * start + '# ' + x for x in lines[1:]]
    return out


def fit(line):
    """line, and what of it does not fit in the width on the lines below
    it, at the column of its comment."""
    if len(line) <= WIDTH or '#' not in line:
        return [line]
    col = line.index('#')
    if line[:col].strip() == '' and '#' in line[col + 1:]:
        # a parameter commented out: its comment is the second one
        col = line.index('#', col + 1)
    out = []
    while len(line) > WIDTH:
        cut = line.rfind(' ', col + 2, WIDTH + 1)
        if cut <= col + 2:
            break
        out.append(line[:cut].rstrip())
        line = ' ' * col + '# ' + line[cut:].strip()
    out.append(line)
    return out


def after_lines(it, col, delta=None):
    """The comment lines right below an element: the indented ones
    continue its comment, at its column (or moved by delta, if given, as
    much as the comment), the others stay at column 0."""
    out, para = [], []

    def flush():
        # the lines of a continuation as they are if they fit at col, else
        # the paragraph they make flowed again at col
        if not para:
            return
        if delta is None:
            lines = [' ' * col + x[1] for x in para]
        else:
            lines = [' ' * max(x[0] + delta, 0) + x[1] for x in para]
        if any(len(l) > WIDTH for l in lines):
            words = ' '.join(x[1][1:].strip() for x in para).split()
            lines, cur = [], ''
            for w in words:
                if cur and col + 2 + len(cur) + 1 + len(w) > WIDTH:
                    lines.append(' ' * col + '# ' + cur)
                    cur = w
                else:
                    cur = (cur + ' ' + w) if cur else w
            lines.append(' ' * col + '# ' + cur)
        out.extend(lines)
        para.clear()

    for a in it.after:
        if a.startswith((' ', '\t')):
            e8 = a.expandtabs(8)
            para.append((len(e8) - len(e8.lstrip()), a.strip()))
        else:
            flush()
            out.append(rule_line(a))
    flush()
    return out


def plain(it, gap=2, drop_structural=False):
    """An element with its comments, the comment gap spaces after the
    tokens."""
    if it.raw:
        lines = [rule_line(b) for b in it.before] + it.raw
        return lines + [rule_line(a) for a in it.after]
    before = [rule_line(b) for b in
              (clean(it.before) if drop_structural else it.before)]
    code = it.text()
    col = len(code) + gap
    lines = wrap_comment(code, it.comment, col)
    after = [a for a in it.after
             if not (drop_structural and STRUCTURAL.match(a.strip()))]
    it2 = Item(it.toks, it.lines)
    it2.after = after
    return before + lines + after_lines(it2, col if it.comment is not None
                                        else 0)


def mode(xs):
    """The most frequent of xs, the smallest of them on a tie (a larger
    column is most often the one a long row pushes)."""
    return min(set(xs), key=lambda x: (-xs.count(x), x)) if xs else None


def table(rows, mincol, minval=4):
    """The rows name [value(s)] # comment in columns: the ones of the
    author (the most frequent among the rows, the commented-out ones
    included), a name too long for them followed by one space and its
    comment by two; else columns wide enough for all. Values that the
    author aligns on their right end stay so."""
    flat = [it for it in rows if not it.raw]
    if not flat:
        out = []
        for it in rows:
            out += [rule_line(b) for b in clean(it.before)] + it.raw + \
                [rule_line(a) for a in it.after]
        return out
    valued = len(flat[0].toks) > 1
    vstart, vend, ccols = [], [], []
    for it in flat:
        o = it.orig
        if valued and len(it.toks) > 1:
            m = re.match(r'\s*\S+(\s+)(\S+(?:\s+\S+)*?)\s*(#|$)', o)
            if m:
                vstart.append(m.start(2))
                vend.append(m.end(2))
        if '#' in o:
            ccols.append(o.index('#'))
        for x in it.before + it.after:
            x = x.expandtabs(8)
            if x.startswith('#') and '#' in x[1:]:
                ccols.append(x.index('#', 1))
    names = [len(it.toks[0]) for it in flat]
    right = False
    if valued:
        w = mode(vstart)
        e = mode(vend)
        if e is not None and vend.count(e) > vstart.count(w) and \
           vend.count(e) > 1:
            right = True     # the values aligned on their right end
        if w is None:
            w = max(mincol, max(names) + 2)
    else:
        w = 0
    c = mode(ccols)

    def prefix_of(it):
        if not valued or len(it.toks) == 1:
            return it.toks[0]
        val = ' '.join(it.toks[1:])
        if right:
            pad = e - len(it.toks[0]) - len(val)
            return it.toks[0] + ' ' * max(pad, 1) + val
        return it.toks[0].ljust(w - 1) + ' ' + val

    pre = {id(it): prefix_of(it) for it in flat}
    if valued:
        fitting = [len(pre[id(it)]) for it in flat if len(it.toks[0]) < w]
    else:
        fitting = [len(pre[id(it)]) for it in flat
                   if c is None or len(pre[id(it)]) < c]
    # the column of the comments: the author's, if one space at least
    # parts it from what fits; else two spaces after the longest
    cmin = max(fitting or [0]) + 1
    if c is None or c < cmin:
        c = cmin + 1
    out = []
    for it in rows:
        out += [rule_line(b) for b in clean(it.before)]
        if it.raw:
            out += it.raw
            out += [rule_line(a) for a in it.after]
            continue
        prefix = pre[id(it)]
        col = c if len(prefix) < c else len(prefix) + 2
        if it.comment is None:
            out.append(prefix)
        else:
            out += wrap_comment(prefix, it.comment, col)
        out += shifted_after(it, col if it.comment is not None else c)
    return out


def shifted_after(it, col):
    """The comment lines right below an element, the indented ones moved
    by as much as its comment moves to col (to col if it had none)."""
    o = it.orig
    if '#' in o and it.comment is not None:
        old = o.index('#')
    else:
        ind = [len(a.expandtabs(8)) - len(a.expandtabs(8).lstrip())
               for a in it.after if a.startswith((' ', '\t'))]
        old = ind[0] if ind else col
    return after_lines(it, col, col - old)


def clean(lines):
    """lines without the structural comments, which the script writes
    itself, and without the empty lines left at the head or two in a row;
    an empty line at the end, which parts the comments from the element,
    stays."""
    out = []
    for x in lines:
        if x and STRUCTURAL.match(x.strip()):
            continue
        if x == '' and (not out or out[-1] == ''):
            continue
        out.append(x)
    return out


def next_item(elems, i):
    """The first Item of the elements from i on."""
    for e in elems[i:]:
        for x in e[1:]:
            if isinstance(x, Item):
                return x
            if isinstance(x, list) and x and isinstance(x[0], Item):
                return x[0]
    return None


def write(elems):
    out = []

    def block(lines):
        while lines and lines[0] == '':
            lines = lines[1:]
        if out and out[-1] != '':
            out.append('')
        out.extend(lines)

    i = 0
    while i < len(elems):
        e = elems[i]
        kind = e[0]
        if kind == 'type':
            it = e[1]
            t = it.toks[0]
            if t in TYPE_COL:
                gap = max(TYPE_COL[t] - len(t), 1)
            else:
                m = re.match(r'\S+( *)#', it.orig)
                gap = len(m.group(1)) if m and m.group(1) else 1
            block(plain(it, gap=gap))
            if e[2] and i + 1 < len(elems) and \
               elems[i + 1][0] == 'number':
                out.extend(plain(elems[i + 1][1]))
                i += 2
                continue
        elif kind == 'ref' and i + 1 < len(elems) and \
                elems[i + 1][0] == 'ref' and not elems[i + 1][1].before and \
                elems[i + 1][1].lines[0] == e[1].lines[-1] + 1:
            # references on consecutive lines: a table, as written
            run = [e[1]]
            j = i + 1
            while j < len(elems) and elems[j][0] == 'ref' and \
                    not elems[j][1].before and \
                    elems[j][1].lines[0] == run[-1].lines[-1] + 1:
                run.append(elems[j][1])
                j += 1
            for it in run:
                if it.text() == '*' and it.comment is None:
                    it.comment = '[none]'
            block(table(run, 0))
            i = j
            continue
        elif kind in ('override', 'number', 'ref', 'raw'):
            it = e[1]
            if kind == 'ref' and it.text() == '*' and it.comment is None:
                it.comment = '[none]'
            block(plain(it))
        elif kind == 'diff':
            block(plain(e[1]))
        elif kind == 'table':
            cnt, k, rows = e[1], e[2], e[3]
            if cnt.comment is None or re.match(
                    r'(the )?(number of )?(integer|int|double|dbl|string|'
                    r'str|vector-of-\w+|vint|vdbl|vstr|vector of \w+)'
                    r'( params| parameters)?\s*$', cnt.comment, re.I):
                cnt.comment = 'number of %s parameters' % k
            block(plain(cnt, drop_structural=True))
            head = ['# now all the %s parameters' % k]
            if rows:
                block(head + table(rows, 16))
            else:
                # the parameters commented out right below the head of an
                # empty table, which the element after it took, are the
                # content of the table
                nxt = next_item(elems, i + 1)
                mine = []
                if nxt is not None:
                    bl = nxt.before
                    cut = bl.index('') if '' in bl else len(bl)
                    mine = [b for b in bl[:cut]
                            if not STRUCTURAL.match(b.strip())]
                    if mine:
                        nxt.before = [b for b in bl[:cut]
                                      if STRUCTURAL.match(b.strip())] + \
                            bl[cut:]
                        while nxt.before and nxt.before[0] == '':
                            nxt.before.pop(0)
                block(head + (mine if mine else ['# [none]']))
        elif kind == 'extra':
            # the comment of the extra slot, written once
            nxt = elems[i + 1][1] if i + 1 < len(elems) else None
            if nxt is not None:
                own = clean(nxt.before)
                if [x for x in own if x]:
                    nxt.before = own   # a comment of the author says it
                else:
                    nxt.before = ['# pointer to the "extra" Configuration']
                if nxt.comment is None or nxt.comment == '':
                    nxt.comment = '[none]' if nxt.text() == '*' else None
        elif kind == 'names':
            lines = table(e[1], 0) if any(it.comment is not None
                                          for it in e[1]) \
                else [x for it in e[1] for x in plain(it)]
            if e[1] and e[1][0].gap:
                block(lines)
            else:
                out.extend(lines)
        elif kind == 'keyed':
            run = []
            j = i
            while j < len(elems) and elems[j][0] == 'keyed':
                run.append(elems[j])
                j += 1
            rows = []
            for _, key, ref in run:
                it = Item([key.toks[0], ref.toks[0]], key.lines)
                it.orig = key.orig
                it.before = key.before + ref.before
                it.comment = ref.comment if ref.comment is not None \
                    else key.comment
                it.after = key.after + ref.after
                rows.append(it)
            block(table(rows, 24, minval=0))
            i = j
            continue
        i += 1
    return out


def box(name, desc):
    def title(t):
        s = (WIDTH - len(t)) // 2
        if s % 2:
            s -= 1
        line = list(RULE)
        line[s - 1] = ' '
        if s + len(t) < WIDTH:
            line[s + len(t)] = ' '
        line[s:s + len(t)] = list(t)
        return ''.join(line).rstrip()
    head = [RULE, title(name), '#'] + desc + ['#', RULE, RULE]
    tail = [RULE, title('END ' + name), RULE]
    return head, tail


def good_box(lines, name):
    """The lines of a box of the head or of the end that are already
    right: 77 columns, the name in them, the dashes on their columns out
    of the title, and one or two spaces on each side of the title (one
    of even length cannot have one on both sides)."""
    for l in lines:
        if len(l) > WIDTH or not l.startswith('# '):
            return False
        m = re.search(r'[^\s-](.*[^\s-])?', l[2:])
        if m is None:
            if l != RULE:
                return False
            continue
        a, b = m.start() + 2, m.end() + 2
        left = l[:a].rstrip(' ')
        right = l[b:].lstrip(' ')
        if not 1 <= a - len(left) <= 2 or left != RULE[:len(left)]:
            return False
        if not right:               # the title at the end of the line
            if b != len(l):
                return False
            continue
        if len(l) != WIDTH or not 1 <= len(l) - b - len(right) <= 2 or \
           right != RULE[WIDTH - len(right):]:
            return False
    return any(name in l for l in lines)


def is_rule(line):
    """A line of dashes, with or without a text in it."""
    return re.match(r'#.*(\s-){5,}\s*-?\s*$', line.strip()) is not None


def split_box(text):
    """The description in the box of the head, the lines that close the
    box (two rules, or more when one of them has a text) and the body."""
    lines = text.split('\n')
    k = 0
    while k < len(lines) and lines[k].startswith('# - - -'):
        k += 1
    desc = []
    while k < len(lines) and lines[k].startswith('#') and \
          not is_rule(lines[k]):
        desc.append(lines[k].rstrip())
        k += 1
    close = []
    while k < len(lines) and lines[k].startswith('#') and is_rule(lines[k]):
        close.append(rule_line(lines[k]))
        k += 1
    while desc and desc[0] == '#':
        desc.pop(0)
    while desc and desc[-1] == '#':
        desc.pop()
    end = len(lines)
    while end > k and (lines[end - 1].startswith('# - - -') or
                       not lines[end - 1].strip()):
        end -= 1
    if len(close) < 2 or all(c == RULE for c in close):
        close = [RULE, RULE]
    return desc, close, '\n'.join(lines[k:end])


def code_tokens(text):
    return [t for line in text.split('\n')
            for t in line.split('#', 1)[0].split()]


def comment_words(text):
    """The words of the comments, the dashes of the rules aside, and
    the ones the script rewrites on purpose aside: the comments of the
    numbers (a count, a flag), which it writes the same everywhere when
    they are a variant of it, and the names in the boxes."""
    out = []
    for line in text.split('\n'):
        if '#' not in line or re.match(r'\s*[-+]?\d+\s+#', line) or \
           (line.startswith('# - - ') and re.search(r'\w', line)):
            continue
        out += [w.strip('-') for w in line.split('#', 1)[1].split()
                if w.strip('-')]
    return sorted(out)


def format_config(path, text, index):
    name = os.path.basename(path)
    if not text.lstrip().startswith('# - - -'):
        raise Skip('no box at the head')
    desc, close, body = split_box(text)
    r = Reader(body)
    if r.peek() is None or is_int(r.peek()) or \
       not re.match(r'[*A-Za-z_][\w:<>,* .-]*$', r.peek()) or \
       ('/' in r.peek() and not r.peek().startswith('*')):
        raise Skip('no Configuration')
    p = Parser(r, path, index)
    p.conf()
    if not r.eof():
        raise Fail('tokens after the Configuration, from line %d: %s'
                   % (r.line_of() + 1, r.peek()))
    lines = [x for l in write(p.out) for x in fit(l)]
    tail = r.tail()
    if tail:
        lines += [''] + [rule_line(t) for t in tail]
    desc = [x for d in desc for x in fit(d)]
    head, foot = box(name, desc)
    head = head[:-2] + close
    old = text.split('\n')
    if len(old) > 1 and good_box([old[1]], name):
        head[1] = old[1]
    ends = [l for l in old if 'END ' + name in l]
    if ends and good_box(ends[-1:], 'END ' + name):
        foot[1] = ends[-1]
    new = '\n'.join(head + [''] + lines + [''] + foot) + '\n'
    if code_tokens(new) != code_tokens(text):
        raise Fail('the tokens would change (a bug of the script)')
    old_words = set(comment_words(text))
    lost = [w for w in old_words if w not in set(comment_words(new))]
    if lost:
        raise Fail('words of the comments would be lost: %s' % lost[:5])
    long = [l for l in new.split('\n') if len(l) > WIDTH]
    if long:
        raise Fail('a line past %d columns: %s' % (WIDTH, long[0]))
    return new


def build_index(files):
    """The configurations of the tree the files are in, by their name."""
    roots = set()
    for f in files:
        d = os.path.dirname(os.path.abspath(f))
        try:
            top = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                                 cwd=d, capture_output=True, text=True,
                                 check=True).stdout.strip()
            sup = subprocess.run(['git', 'rev-parse',
                                  '--show-superproject-working-tree'],
                                 cwd=d, capture_output=True,
                                 text=True).stdout.strip()
            roots.add(sup or top)
        except (subprocess.CalledProcessError, FileNotFoundError):
            roots.add(d)
    index = {}
    for root in roots:
        for dp, dn, fn in os.walk(root):
            dn[:] = [x for x in dn if x != '.git' and
                     not x.startswith('build')]
            for x in fn:
                if x.endswith('.txt'):
                    index.setdefault(x, []).append(os.path.join(dp, x))
    return index


def usage():
    """The part of the documentation that says how to run the script."""
    return __doc__[__doc__.index('Usage:'):].rstrip()


def main(argv):
    if '-h' in argv or '--help' in argv:
        print(usage())
        return 0
    check = '--check' in argv
    show = '--diff' in argv
    files = [a for a in argv if a not in ('--check', '--diff')]
    unknown = [a for a in files if a.startswith('-')]
    if unknown:
        print('cfgformat.py: unknown option %s\n%s' % (unknown[0], usage()),
              file=sys.stderr)
        return 2
    index = build_index(files)
    changed, failed = [], []
    for path in files:
        try:
            text = open(path, encoding='utf-8').read()
            new = format_config(path, text, index)
        except UnicodeDecodeError:
            continue   # no text, hence no configuration
        except Skip:
            continue
        except Fail as e:
            failed.append((path, str(e)))
            continue
        except (ValueError, IndexError, TypeError, AttributeError) as e:
            failed.append((path, 'the script could not read it (%s: %s)'
                           % (type(e).__name__, e)))
            continue
        if new != text:
            changed.append(path)
            if show:
                import difflib
                sys.stdout.writelines(difflib.unified_diff(
                    text.splitlines(True), new.splitlines(True),
                    path, path))
                continue
            if not check:
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(new)
    if not show:
        for p in changed:
            print(p)
    for p, why in failed:
        print('%s: left as it is: %s' % (p, why), file=sys.stderr)
    return 1 if (check and changed) else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
