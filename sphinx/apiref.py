#!/usr/bin/env python3
# --------------------------------------------------------------------------- #
#                                 apiref.py                                   #
#                                                                             #
# Writes the API reference of the Sphinx site into sphinx/api from the XML    #
# that doxygen/Doxyfile writes into doc/xml, as a tree that mirrors the       #
# project: a page with the modules (the core library, the Blocks, the         #
# Solvers and the tools), a page per module, in it a page per group of        #
# headers (the Blocks, the Functions, the Solvers, ... of that module), a     #
# page per header and a page per class, each rendered by Breathe. A header    #
# goes to the group of its main class, which is found from the base classes   #
# of the core (every class derived from Block is under the Blocks, and so     #
# on); in a module, the Blocks derived from a Block of the same module go to  #
# a group of their own (the unit Blocks of UCBlock), and HEADERS below        #
# places the headers that the inheritance does not place well. pages.py       #
# calls it, after Doxygen and before Sphinx.                                  #
# --------------------------------------------------------------------------- #

import collections
import glob
import hashlib
import os
import re
import shutil
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
XML = os.path.join(ROOT, 'doc', 'xml')
OUT = os.path.join(ROOT, 'sphinx', 'api')
CORE = 'SMS++'
NAMESPACE = 'SMSpp_di_unipi_it::'
GITLAB = 'https://gitlab.com/smspp/'

# where the pages of the Doxygen site are, seen from a page of a module,
# which lies in sphinx/api/<module> on a site served in a subdirectory of
# the Doxygen one (public/sphinx next to public/index.html)
DOXYGEN = '../../../'

# the groups of headers, in the order of the pages, each with the base
# class of the core that brings a class into it; the first base met going
# up from a class decides
ROLES = [
    ('Block', 'Blocks'),
    ('Variable', 'Variables'),
    ('Constraint', 'Constraints'),
    ('Objective', 'Objectives'),
    ('Function', 'Functions'),
    ('Modification', 'Modifications'),
    ('Solver', 'Solvers'),
    ('Configuration', 'Configurations'),
    ('Solution', 'Solutions'),
]
UTILITIES = 'Data and utilities'
DOCUMENTATION = 'Documentation'

# the last word of the name of a header that gives its group, before its
# classes are looked at
SUFFIXES = [
    ('Block', 'Blocks'), ('Variable', 'Variables'),
    ('Constraint', 'Constraints'), ('Objective', 'Objectives'),
    ('Function', 'Functions'), ('Solver', 'Solvers'),
    ('Config', 'Configurations'), ('Configuration', 'Configurations'),
    ('Solution', 'Solutions'), ('Modification', 'Modifications'),
]

# the headers that neither their name nor their classes place well
HEADERS = {
    'SMS++/include/BlockInspection.h': 'Blocks',
    'SMS++/include/Change.h': 'Modifications',
    'SMS++/include/AbstractChange.h': 'Modifications',
}

# the one line of a module, when the list of the projects in README.md has
# none for it
LINES = {
    'tools': 'the executables that solve the Blocks of the modules from '
             'their files, and the utility that edits configuration files.',
}


# ----- reading the XML ----------------------------------------------------- #

def text(node):
    """the text of a node of the XML, on one line"""
    if node is None:
        return ''
    return ' '.join(''.join(node.itertext()).split())


def first_sentence(line):
    m = re.search(r'(?<!\be\.g)(?<!\bi\.e)(?<!\bvs)\.(\s|$)', line)
    return line[:m.start() + 1] if m else line


def description(cd):
    """the brief description of a compound, else the first sentence of the
    first paragraph of its detailed one that is not a section"""
    brief = text(cd.find('briefdescription'))
    if brief:
        return brief
    detail = cd.find('detaileddescription')
    if detail is None:
        return ''
    for para in detail.findall('para'):
        para = ET.fromstring(ET.tostring(para))
        for sub in list(para):
            if sub.tag in ('simplesect', 'parameterlist', 'xrefsect'):
                para.remove(sub)
        line = text(para)
        if line:
            return first_sentence(line)
    return ''


def scope(name):
    return name.rpartition('::')[0]


def short(name):
    return name[len(NAMESPACE):] if name.startswith(NAMESPACE) else name


def html_page(refid):
    """the page of the Doxygen site of a compound, which CREATE_SUBDIRS puts
    in d<x>/d<yy>, two digits of the MD5 of its name"""
    base = refid.rsplit('/', 1)[-1]
    md5 = hashlib.md5(base.encode()).digest()
    return 'd%x/d%02x/%s.html' % (md5[14] & 0xf, md5[15], base)


class Compound:
    def __init__(self, cd):
        self.refid = cd.get('id').rsplit('/', 1)[-1]
        self.kind = cd.get('kind')
        self.name = cd.findtext('compoundname')
        self.short = short(self.name)
        loc = cd.find('location')
        self.file = loc.get('file') if loc is not None else ''
        self.line = int(loc.get('line') or 0) if loc is not None else 0
        self.brief = description(cd)
        self.bases = [b.get('refid').rsplit('/', 1)[-1]
                      for b in cd.findall('basecompoundref') if b.get('refid')]
        self.derived = [b.get('refid').rsplit('/', 1)[-1]
                        for b in cd.findall('derivedcompoundref')
                        if b.get('refid')]
        self.inner = [i.get('refid').rsplit('/', 1)[-1]
                      for i in cd.findall('innerclass')]
        self.title = cd.findtext('title') or ''


def read_xml():
    """the compounds and the members outside classes, which lie in the
    compounds of the namespaces, of the groups and of the files"""
    compounds, members = {}, {}
    for path in glob.glob(os.path.join(XML, '**', '*.xml'), recursive=True):
        if os.path.basename(path) in ('index.xml', 'Doxyfile.xml'):
            continue
        root = ET.parse(path).getroot()
        for cd in root.findall('compounddef'):
            c = Compound(cd)
            compounds[c.refid] = c
            if c.kind not in ('namespace', 'group', 'file'):
                continue
            for m in cd.iter('memberdef'):
                loc = m.find('location')
                mid = m.get('id').rsplit('/', 1)[-1]
                if loc is None or mid in members:
                    continue
                if m.get('kind') == 'friend':
                    continue
                members[mid] = {
                    'kind': m.get('kind'), 'file': loc.get('file'),
                    'line': int(loc.get('line') or 0),
                    'name': m.findtext('name'),
                    'qualified': m.findtext('qualifiedname')
                    or m.findtext('name'),
                    'args': m.findtext('argsstring') or '',
                    'scope': c.name if c.kind == 'namespace' else ''}
    return compounds, members


# ----- the modules --------------------------------------------------------- #

def module_of(path):
    """the module of a file of the reference: the directory that holds its
    include/"""
    m = re.match(r'([^/]+)/include/', path)
    return m.group(1) if m else ''


def slug(module):
    return module.replace('+', 'p')


def readme_lines():
    """the one line of each module from the list of the projects in
    README.md: the first sentence of its item, without the Markdown links"""
    with open(os.path.join(ROOT, 'README.md')) as f:
        readme = f.read()
    part = readme.split('## Current projects', 1)[-1].split('\n## ', 1)[0]
    lines = {}
    for item in re.split(r'\n\s*\n', part):
        m = re.match(r'\s*- \[([^\]]+)\]\((https?://[^)]+)\),?\s*(.*)',
                     item, re.S)
        if not m:
            continue
        key = re.sub(r'[^a-z0-9]', '',
                     m.group(2).rstrip('/').rsplit('/', 1)[-1].lower())
        body = re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', ' '.join(
            m.group(3).split()))
        line = first_sentence(body)
        lines[key] = (m.group(2), line[:1].upper() + line[1:])
    return lines


def rst_inline(line):
    """a line of Markdown as reStructuredText: the code in double quotes,
    the rest escaped"""
    parts = line.split('`')
    out = []
    for k, part in enumerate(parts):
        if k % 2:
            out.append('``%s``' % part)
        else:
            out.append(re.sub(r'([*_|\\\[\]<>])', r'\\\1', part))
    return ''.join(out)


def escape(line):
    return re.sub(r'([*_|`\\\[\]<>])', r'\\\1', line)


# ----- the groups ---------------------------------------------------------- #

def ancestors(c, compounds):
    """the class and its base classes, nearest first"""
    out, queue, seen = [], [c.refid], set()
    while queue:
        r = queue.pop(0)
        if r in seen or r not in compounds:
            continue
        seen.add(r)
        out.append(compounds[r])
        queue.extend(compounds[r].bases)
    return out


def depth(c, compounds, memo={}):
    if c.refid not in memo:
        memo[c.refid] = 0
        memo[c.refid] = max([depth(compounds[b], compounds) + 1
                             for b in c.bases if b in compounds] or [0])
    return memo[c.refid]


def role(c, compounds):
    names = dict(ROLES)
    for a in ancestors(c, compounds):
        if a.short in names and module_of(a.file) == CORE:
            return names[a.short]
    return None


def plural(name):
    words = re.findall(r'[A-Z]+(?![a-z])|[A-Z][a-z0-9]*|[a-z0-9]+', name)
    return ' '.join(words) + 's'


def family(c, compounds):
    """the topmost Block of the same module from which a Block of a module
    other than the core derives, or None"""
    module, top = module_of(c.file), None
    for a in ancestors(c, compounds)[1:]:
        if module_of(a.file) == module and role(a, compounds) == 'Blocks':
            top = a
    return top


class Header:
    def __init__(self, f, classes):
        self.file = f.file
        self.name = os.path.basename(f.file)
        self.stem = os.path.splitext(self.name)[0]
        self.refid = f.refid
        self.brief = f.brief
        self.classes = classes
        self.members = []
        self.page = re.sub(r'\W', '_', self.name)
        self.group = None
        self.order = (99, self.name)


def headers_of(compounds, members):
    """the headers of each module, with their classes and members"""
    nested = {i for c in compounds.values() if c.kind in ('class', 'struct',
                                                          'union')
              for i in c.inner}
    by_file = collections.defaultdict(list)
    for c in compounds.values():
        if c.kind in ('class', 'struct', 'union') and c.refid not in nested:
            by_file[c.file].append(c)
    # the members of the groups include those of classes, which the pages
    # of the classes show, and the specializations of the templates, which
    # Breathe cannot find by name
    spaces = {''} | {c.name for c in compounds.values()
                     if c.kind == 'namespace'}
    modules = collections.defaultdict(list)
    for f in compounds.values():
        if f.kind != 'file' or not module_of(f.file):
            continue
        stem = os.path.splitext(os.path.basename(f.file))[0]
        h = Header(f, sorted(by_file.get(f.file, []),
                             key=lambda c: (c.short != stem, c.line)))
        h.members = sorted((m for m in members.values()
                            if m['file'] == f.file
                            and scope(m['qualified']) in spaces
                            and '<' not in m['qualified']),
                           key=lambda m: m['line'])
        if not h.brief:
            main = [c for c in h.classes if c.short == h.stem]
            h.brief = main[0].brief if main else ''
        modules[module_of(f.file)].append(h)
    for h in [h for hs in modules.values() for h in hs]:
        place(h, compounds)
    return modules


def place(h, compounds):
    """the group of a header: the one its name says, else the one of its
    main class, which is the class named as the header or else the most
    common among its classes"""
    module = module_of(h.file)
    main = [c for c in h.classes if c.short == h.stem] or h.classes
    suffix = [g for w, g in SUFFIXES if h.stem.endswith(w)]
    if h.file in HEADERS:
        h.group = HEADERS[h.file]
    elif suffix:
        h.group = suffix[0]
    else:
        roles = collections.Counter(r for r in (role(c, compounds)
                                                for c in main) if r)
        h.group = roles.most_common(1)[0][0] if roles else UTILITIES
    if h.group == 'Blocks' and module != CORE and main:
        top = family(main[0], compounds)
        if top is None and any(module_of(compounds[d].file) == module
                               for d in main[0].derived if d in compounds):
            top = main[0]
        if top is not None:
            h.group = plural(top.short)
    if main:
        h.order = (min(depth(c, compounds) for c in main), h.name)


def capital(line):
    return line[:1].upper() + line[1:]


def summary(h):
    """the line of a header in the tables: the brief description of its
    main class, which is shorter than the first sentence of the comment of
    the file, else that sentence without its "Header file for"
    """
    main = [c for c in h.classes if c.short == h.stem and c.brief]
    if main:
        return capital(main[0].brief)
    return capital(re.sub(r'^Header file (for|with|defining) (the )?', '',
                          h.brief))


def group_order(group):
    titles = [t for _, t in ROLES]
    if group == 'Blocks':
        return (0, '')
    if group.endswith('Blocks'):
        return (1, group)
    if group in titles:
        return (2 + titles.index(group), '')
    return (100 if group == UTILITIES else 101, group)


# ----- the pages ----------------------------------------------------------- #

def title(line, char='='):
    return '%s\n%s\n\n' % (line, char * len(line))


def rubric(line):
    # a heading that stays out of the table of contents, where the pages
    # below a page go and its sections would come between them
    return '.. rubric:: %s\n\n' % line


def toctree(entries, hidden=True, maxdepth=1):
    out = '.. toctree::\n   :maxdepth: %d\n' % maxdepth
    if hidden:
        out += '   :hidden:\n'
    return out + '\n' + ''.join('   %s\n' % e for e in entries) + '\n'


def table(rows, header):
    if not rows:
        return ''
    out = '.. list-table::\n   :header-rows: 1\n   :widths: 30 70\n\n'
    for row in [header] + rows:
        out += '   * - %s\n     - %s\n' % (row[0], row[1] or ' ')
    return out + '\n'


def class_page(c, h, module, compounds):
    kind = 'struct' if c.kind == 'struct' else 'class'
    out = title(escape(c.short))
    out += ('Declared in :doc:`%s <%s>` (module :doc:`%s <index>`); the '
            'same %s on the `Doxygen site <%s%s>`__.\n\n'
            % (h.name, h.page, module, kind, DOXYGEN, html_page(c.refid)))
    if c.bases or c.derived:
        # the diagram of the hierarchy that the Doxygen site draws for the
        # class, which leads to its page there
        out += ('.. raw:: html\n\n   <p><a href="%s%s"><img alt="inheritance'
                ' diagram of %s" src="%s%s"></a></p>\n\n'
                % (DOXYGEN, html_page(c.refid), c.short, DOXYGEN,
                   html_page(c.refid)[:-5] + '.png'))
    out += ('.. doxygen%s:: %s\n   :members:\n   :protected-members:\n'
            '   :undoc-members:\n' % (kind, c.name))
    return out


def member_directive(m):
    name = m['qualified']
    if m['kind'] == 'function':
        args = re.match(r'\(.*\)', m['args'])
        return '.. doxygenfunction:: %s%s\n' % (name,
                                                 args.group(0) if args else '')
    kind = {'typedef': 'typedef', 'enum': 'enum', 'variable': 'variable',
            'define': 'define'}.get(m['kind'])
    return '.. doxygen%s:: %s\n' % (kind, name) if kind else ''


def header_page(h, module, pages):
    out = title(escape(h.name))
    out += ('Header of module :doc:`%s <index>`, in the group :doc:`%s <%s>`;'
            ' the same header on the `Doxygen site <%s%s>`__.\n\n'
            % (module, h.group, group_page(h.group), DOXYGEN,
               html_page(h.refid)))
    if h.brief:
        out += escape(h.brief) + '\n\n'
    if h.classes:
        out += rubric('Classes')
        out += table([(':doc:`%s <%s>`' % (escape(c.short), pages[c.refid]),
                       escape(capital(c.brief))) for c in h.classes],
                     ('Class', 'Description'))
        out += toctree([pages[c.refid] for c in h.classes])
    kinds = [('function', 'Functions'), ('typedef', 'Types'),
             ('enum', 'Enumerations'), ('variable', 'Variables'),
             ('define', 'Macros')]
    for kind, name in kinds:
        ms = [m for m in h.members if m['kind'] == kind]
        if kind == 'define':
            # the include guard and the other macros nobody documented
            ms = [m for m in ms if not m['name'].endswith(('_H', '_INCLUDED'))]
        if ms:
            out += rubric(name)
            out += ''.join(member_directive(m) + '\n' for m in ms)
    # the whole comment of the file, authors and copyright included, last
    out += rubric('Description of the header')
    out += ('.. doxygenfile:: %s\n   :sections: briefdescription '
            'detaileddescription\n' % h.name)
    return out


def group_page(group):
    return 'group_' + re.sub(r'\W+', '_', group.lower())


def write(page, content):
    full = os.path.join(OUT, page + '.rst')
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(content)


def tools_items():
    """the items of the list of the executables in the README.md of the
    tools, on one line each"""
    try:
        with open(os.path.join(ROOT, 'tools', 'README.md')) as f:
            readme = f.read()
    except OSError:
        return []
    part = readme.split('At the moment we provide:', 1)[-1]
    return [' '.join(i.split()) for i in re.findall(
        r'^- (.*(?:\n  .*)*)', part.split('\n## ', 1)[0], re.M)]


def tools_list():
    return ''.join('- %s\n' % rst_inline(i) for i in tools_items()) + '\n'


def start(module, groups, pages):
    """the paragraph "where to start" of a module: its main Blocks, its
    Solvers, and the tool that solves its Blocks from a file, with the
    configuration files of the tool as examples"""
    def links(group, most):
        cs = [c for h in groups.get(group, []) for c in h.classes
              if c.short == h.stem]
        cs.sort(key=lambda c: c.short != module)
        return ', '.join(':doc:`%s <%s>`' % (escape(c.short), pages[c.refid])
                         for c in cs[:most])
    blocks = ', '.join(x for x in [links(g, 2) for g in groups
                                   if g.endswith('Blocks')] if x)
    solvers = links('Solvers', 4)
    lines = []
    if blocks:
        lines.append('Blocks: %s' % blocks)
    if solvers:
        lines.append('Solvers: %s' % solvers)
    for item in tools_items():
        m = re.match(r'`(\w+)`: an? (\w+) solver', item)
        if not m or m.group(2) != module:
            continue
        tool = m.group(1)
        if os.path.isdir(os.path.join(ROOT, 'tools', tool, 'config')):
            lines.append('tool: ``%s``, which solves a %s read from a file;'
                         ' its `configuration files <%stools/-/tree/'
                         'develop/%s/config>`__ are examples'
                         % (tool, module, GITLAB, tool))
    if not lines:
        return ''
    return (rubric('Where to start') +
            ''.join('- %s\n' % line for line in lines) + '\n')


def kind_of(module):
    if module == CORE:
        return 'Core library'
    if module == 'tools':
        return 'Tools'
    if module.endswith('Block'):
        return 'Blocks'
    return 'Solvers'


def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    if not os.path.isdir(XML):
        write('index', title('API Reference') + 'Doxygen has not run.\n')
        return
    compounds, members = read_xml()
    modules = headers_of(compounds, members)
    readme = readme_lines()
    pages = {}
    for module, hs in modules.items():
        for h in hs:
            for c in h.classes:
                pages[c.refid] = 'class_' + re.sub(r'\W+', '_',
                                                   c.short).strip('_')
    docs = collections.defaultdict(list)
    for c in compounds.values():
        if c.kind == 'page' and module_of(c.file):
            docs[module_of(c.file)].append(c)
    if os.path.isdir(os.path.join(ROOT, 'tools')):
        modules.setdefault('tools', [])

    order = sorted(modules, key=lambda m: (['Core library', 'Blocks',
                                            'Solvers', 'Tools']
                                           .index(kind_of(m)), m.lower()))
    index = title('API Reference')
    index += ('The classes of the core library and of each module, the '
              'headers of each module gathered by what they define (its '
              'Blocks, its Functions, its Solvers, ...); the same reference, '
              'with the lists of all the classes and of all the files, is '
              'on the `Doxygen site <%s>`__.\n\n' % DOXYGEN[3:])
    for kind in ['Core library', 'Blocks', 'Solvers', 'Tools']:
        rows = []
        for module in [m for m in order if kind_of(m) == kind]:
            url, line = readme.get(slug(module).lower(),
                                   ('%s%s' % (GITLAB, module.lower()),
                                    LINES.get(module, '')))
            rows.append((':doc:`%s <%s/index>`' % (escape(module),
                                                    slug(module)),
                         rst_inline(line)))
        if rows:
            index += rubric(kind) + table(rows, ('Module', 'Description'))
    index += toctree(['%s/index' % slug(m) for m in order])
    write('index', index)

    for module in order:
        hs = sorted(modules[module], key=lambda h: h.order)
        url, line = readme.get(slug(module).lower(),
                               ('%s%s' % (GITLAB, module.lower()),
                                LINES.get(module, '')))
        out = title(escape(module))
        out += rst_inline(line) + '\n\n'
        out += 'The repository of the module is `%s <%s>`__.\n\n' % (url, url)
        if module == 'tools':
            out += tools_list()
        groups = collections.OrderedDict()
        for h in sorted(hs, key=lambda h: group_order(h.group)):
            groups.setdefault(h.group, []).append(h)
        out += start(module, groups, pages)
        if docs.get(module):
            groups[DOCUMENTATION] = []
        for group, members_of in groups.items():
            out += rubric(':doc:`%s <%s>`' % (group, group_page(group)))
            if group == DOCUMENTATION:
                out += table([(':doc:`%s <%s>`' % (escape(p.title),
                                                   'page_' + p.refid), '')
                              for p in docs[module]], ('Page', ''))
                for p in docs[module]:
                    write('%s/page_%s' % (slug(module), p.refid),
                          title(escape(p.title)) +
                          '.. doxygenpage:: %s\n   :content-only:\n'
                          % p.refid)
                write('%s/%s' % (slug(module), group_page(group)),
                      title(group) + table(
                          [(':doc:`%s <%s>`' % (escape(p.title),
                                                'page_' + p.refid), '')
                           for p in docs[module]], ('Page', '')) +
                      toctree(['page_' + p.refid for p in docs[module]]))
                continue
            rows = [(':doc:`%s <%s>`' % (escape(h.name), h.page),
                     escape(summary(h))) for h in members_of]
            out += table(rows, ('Header', 'Description'))
            gp = title(group)
            gp += '*headers of module* :doc:`%s <index>`\n\n' % module
            for h in members_of:
                gp += rubric(':doc:`%s <%s>`' % (escape(h.name), h.page))
                gp += escape(h.brief) + '\n\n'
                gp += table([(':doc:`%s <%s>`' % (escape(c.short),
                                                   pages[c.refid]),
                              escape(capital(c.brief)))
                             for c in h.classes], ('Class', 'Description'))
                write('%s/%s' % (slug(module), h.page),
                      header_page(h, module, pages))
                for c in h.classes:
                    write('%s/%s' % (slug(module), pages[c.refid]),
                          class_page(c, h, module, compounds))
            gp += toctree([h.page for h in members_of])
            write('%s/%s' % (slug(module), group_page(group)), gp)
        out += toctree([group_page(g) for g in groups])
        write('%s/index' % slug(module), out)


if __name__ == '__main__':
    main()

# --------------------------------------------------------------------------- #
#                             End of apiref.py                                #
# --------------------------------------------------------------------------- #
