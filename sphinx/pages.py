#!/usr/bin/env python3
# --------------------------------------------------------------------------- #
#                                 pages.py                                    #
#                                                                             #
# Writes the pages of the Sphinx site into sphinx/generated from the          #
# Markdown files of the umbrella, which stay the one source of the text:      #
# each section of the README.md becomes a page of the site, the sections of   #
# the general description in doxygen/mainpage.md become the user guide,       #
# CONTRIBUTING.md and CHANGELOG.md become the pages of the project, and the   #
# links to the files of the repository point to GitLab; then apiref.py        #
# writes the API reference into sphinx/api. The pages job of .gitlab-ci.yml   #
# runs it after Doxygen and before Sphinx:                                    #
#                                                                             #
#   python3 sphinx/pages.py                                                   #
#   sphinx-build -b html -d doc/doctrees sphinx public/sphinx                 #
# --------------------------------------------------------------------------- #

import os
import re
import shutil

import apiref

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'sphinx', 'generated')
REPO = 'https://gitlab.com/smspp/smspp-project/-/'
LOGO = 'SMSpp_logo_mid_noback.png'

# the sections of the README and the page each one goes to, with the title
# of the page when several sections share it; the sections not listed here
# go to the home page, and those mapped to None are left out
README = {
    'Current projects': ('user-guide/modules.md', None),
    'Getting started': ('getting-started/installation.md', None),
    'First steps': ('getting-started/first-steps.md', None),
    'Getting help': ('project/help.md', None),
    'Contributing': (None, None),
    'Authors': ('project/about.md', 'About'),
    'License': ('project/about.md', 'About'),
    'Disclaimer': ('project/about.md', 'About'),
    'Acknowledgements': ('project/about.md', 'About'),
}

# the subsections of "A general description of SMS++" in doxygen/mainpage.md
MAINPAGE = {
    'Design Objectives of SMS++': 'user-guide/design-objectives.md',
    'Alternatives to SMS++': 'user-guide/alternatives.md',
    'Main features of SMS++': 'user-guide/main-features.md',
    'Long-term Objectives of SMS++': 'user-guide/long-term-objectives.md',
}

# the files of the repository that are pages of the site
LOCAL = {
    'CONTRIBUTING.md': 'project/contributing.md',
}


def links(text, page):
    """the relative links of the Markdown made absolute on GitLab"""
    up = '../' * page.count('/')

    def fix(m):
        target = m.group(2)
        if re.match(r'[a-z]+:', target) or target.startswith('#'):
            return m.group(0)
        if os.path.basename(target) == LOGO:
            return '%s(%s%s)' % (m.group(1), up, LOGO)
        if target in LOCAL:
            return '%s(%s%s)' % (m.group(1), up, LOCAL[target])
        first = target.split('/')[0]
        if (not os.path.exists(os.path.join(ROOT, target))
                and re.fullmatch(r'[\w-]+(\.[\w-]+)+', first)):
            return '%s(https://%s)' % (m.group(1), target)
        kind = 'tree' if target.endswith('/') else 'blob'
        return '%s(%s%s/develop/%s)' % (m.group(1), REPO, kind, target)
    return re.sub(r'(\]|\]\s)\(([^)\s]+)\)', fix, text)


def promote(text, levels):
    """the headings raised by the given levels, outside the code blocks"""
    out, fence = [], False
    for line in text.splitlines(keepends=True):
        if line.lstrip().startswith('```'):
            fence = not fence
        m = re.match(r'(#+) ', line)
        if m and not fence and len(m.group(1)) > levels:
            line = line[levels:]
        out.append(line)
    return ''.join(out)


def sections(text, level):
    """the head of the text and its sections at the given heading level"""
    parts = re.split(r'^%s ' % ('#' * level), text, flags=re.M)
    return parts[0], [p.partition('\n')[::2] for p in parts[1:]]


def write(page, text):
    full = os.path.join(OUT, page)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(links(text, page))


def read(name):
    with open(os.path.join(ROOT, name)) as f:
        return f.read()


def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    shutil.copy(os.path.join(ROOT, 'doxygen', LOGO), OUT)

    head, parts = sections(read('README.md'), 2)
    home, pages = [head], {}
    for title, body in parts:
        page, name = README.get(title.strip(), ('index.md', None))
        if page == 'index.md':
            home.append('## %s\n%s' % (title, body))
        elif page:
            if name:
                text = pages.get(page, '# %s\n\n' % name)
                pages[page] = text + '## %s\n%s' % (title, body)
            else:
                pages[page] = '# %s\n%s' % (title, promote(body, 1))
    write('index.md', ''.join(home))
    for page, text in pages.items():
        write(page, text)

    _, parts = sections(read(os.path.join('doxygen', 'mainpage.md')), 2)
    general = dict(parts)['A general description of SMS++']
    _, parts = sections(general, 3)
    for title, body in parts:
        write(MAINPAGE[title.strip()], '# %s\n%s' % (title, promote(body, 2)))

    for name, page in LOCAL.items():
        write(page, read(name))
    log = read('CHANGELOG.md').replace('# Changelog', '# Release Notes', 1)
    write('project/release_notes.md', log)

    apiref.main()


if __name__ == '__main__':
    main()

# --------------------------------------------------------------------------- #
#                              End of pages.py                                #
# --------------------------------------------------------------------------- #
