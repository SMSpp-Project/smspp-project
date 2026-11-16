# --------------------------------------------------------------------------- #
#                                 hooks.py                                    #
#                                                                             #
# The hooks of MkDocs for the web site of the manual. Before the Markdown of  #
# a page is converted, (i) each displayed formula between two lines $$ gets   #
# a blank line before and after, so that pymdownx.arithmatex takes it as a    #
# block and hands it to KaTeX untouched (Pandoc, which builds the PDF, also   #
# takes a display inside a paragraph, and the Markdown of MkDocs would read   #
# \{, * and _ in it), and (ii) the name of a file or folder of the project    #
# written as code and not yet within a link, e.g., `SMS++/include/Block.h`,   #
# becomes a link to it in the GitLab repository of its submodule, as          #
# build/source-links.lua does in the PDF. The files are those that git        #
# tracks under SMSPP_ROOT (by default the umbrella of this file), with the    #
# submodules checked out.                                                     #
# --------------------------------------------------------------------------- #

import os
import re
import subprocess

REPO = 'https://gitlab.com/smspp/smspp-project'  # the umbrella repository
BRANCH = 'develop'                               # the branch published
ROOT = os.environ.get('SMSPP_ROOT') or os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))))
SPAN = re.compile(r'(?<!`)`([A-Za-z0-9_.+][A-Za-z0-9_.+/-]*)`(?!`)')
LINK = re.compile(r'\[(?:[^\[\]]|\[[^\]]*\])*\]\([^)]*\)')
_tracked, _repos = None, None


def repositories():
    """the folder of each submodule on GitLab -> the URL of its repository"""
    global _repos
    if _repos is None:
        _repos = {'': REPO}
        path = None
        try:
            lines = open(os.path.join(ROOT, '.gitmodules')).read().split('\n')
        except OSError:
            lines = []
        for line in lines:
            k, _, v = line.strip().partition(' = ')
            if k == 'path':
                path = v
            elif k == 'url' and path:
                url = v[:-4] if v.endswith('.git') else v
                if url.startswith('../'):
                    url = REPO.rsplit('/', 1)[0] + url[2:]
                if url.startswith('https://gitlab.com/'):
                    _repos[path] = url
    return _repos


def tracked():
    """the files that git tracks in the umbrella and in its submodules"""
    global _tracked
    if _tracked is None:
        try:
            _tracked = set(subprocess.run(
                ['git', '-C', ROOT, 'ls-files', '--recurse-submodules'],
                capture_output=True, text=True, check=True).stdout.split())
        except (OSError, subprocess.CalledProcessError):
            _tracked = set()
    return _tracked


def source_url(path):
    """the GitLab URL of a file or folder of the project, or None"""
    if '/' not in path or '..' in path or path.startswith('/'):
        return None
    name = path.rstrip('/')
    if name in tracked():
        kind = 'blob'
    elif any(f.startswith(name + '/') for f in tracked()):
        kind = 'tree'
    else:
        return None
    sub = max((s for s in repositories()
               if s == '' or name == s or name.startswith(s + '/')), key=len)
    rest = name[len(sub):].lstrip('/')
    url = f'{repositories()[sub]}/-/{kind}/{BRANCH}'
    return f'{url}/{rest}' if rest else url


def link_sources(line):
    def repl(m):
        url = source_url(m.group(1))
        return f'[`{m.group(1)}`]({url})' if url else m.group(0)
    out, last = [], 0
    for m in LINK.finditer(line):  # the names already within a link stay
        out += [SPAN.sub(repl, line[last:m.start()]), m.group(0)]
        last = m.end()
    return ''.join(out) + SPAN.sub(repl, line[last:])


def on_page_markdown(markdown, **kwargs):
    out, fence, disp = [], None, False
    for line in markdown.split('\n'):
        s = line.strip()
        m = re.match(r'(`{3,}|~{3,})', s)
        if fence:
            out.append(line)
            if m and s.startswith(fence):
                fence = None
            continue
        if m:
            fence = m.group(1)
            out.append(line)
            continue
        if not disp and len(s) > 4 and s.startswith('$$') and \
           s.endswith('$$'):
            ind = line[:len(line) - len(line.lstrip())]
            if out and out[-1].strip():
                out.append(ind)
            out += [line, ind]
            continue
        if s == '$$':
            ind = line[:len(line) - len(line.lstrip())]
            if not disp:
                if out and out[-1].strip():
                    out.append(ind)
                out.append(line)
            else:
                out.append(line)
                out.append(ind)
            disp = not disp
            continue
        out.append(line if disp else link_sources(line))
    return '\n'.join(out)
