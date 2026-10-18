# --------------------------------------------------------------------------- #
#                                 conf.py                                     #
#                                                                             #
# The configuration of Sphinx for the second site of the documentation of     #
# SMS++, next to the one of Doxygen and to the user manual: the pages that    #
# sphinx/pages.py writes from the Markdown files of the umbrella (read by     #
# MyST), and the API reference, which Exhale builds from the XML that         #
# doxygen/Doxyfile writes into doc/xml and Breathe renders; Doxygen is run    #
# before, and not by Exhale. The theme is that of Read the Docs. The pages    #
# job of .gitlab-ci.yml builds it with                                        #
#                                                                             #
#   doxygen doxygen/Doxyfile                                                  #
#   python3 sphinx/pages.py                                                   #
#   sphinx-build -b html -d doc/doctrees sphinx public/sphinx                 #
# --------------------------------------------------------------------------- #

import os
import re
import shutil
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def derive_version():
    """the version as cmake/DeriveVersion.cmake derives it: the last tag of
    git, else the VERSION.txt that git archive fills in, else 0.0.0"""
    raw = ''
    try:
        raw = subprocess.run(['git', 'describe', '--tags', '--abbrev=0'],
                             cwd=ROOT, capture_output=True, text=True,
                             check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        pass
    if not raw:
        try:
            with open(os.path.join(ROOT, 'VERSION.txt')) as f:
                raw = f.read().strip()
        except OSError:
            pass
    m = re.match(r'v?(\d+(\.\d+)*)', raw)
    return m.group(1) if m else '0.0.0'


project = 'SMS++'
author = 'Antonio Frangioni and Donato Meoli'
copyright = author
release = derive_version()
version = release

extensions = [
    'breathe',              # the Doxygen XML in Sphinx
    'exhale',               # a page per class, function and file
    'myst_parser',          # the pages in Markdown
    'sphinx.ext.mathjax',   # the formulae of the comments
]

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# what the comments of the headers cause and the configuration cannot mend:
# a nested class is rendered both in its outer class, which Breathe does
# for every :members:, and in the page Exhale gives it, and the \verbatim
# blocks with formulae fail to be lexed as C++
suppress_warnings = ['duplicate_declaration.cpp', 'misc.highlighting_failure']

breathe_projects = {'SMS++': os.path.join(ROOT, 'doc', 'xml')}
breathe_default_project = 'SMS++'
primary_domain = 'cpp'
highlight_language = 'cpp'

def mend_xml():
    """the XML of doxygen/Doxyfile made fit for Breathe and Exhale, in place
    (it is a product of the build, which is read and never published):

    - the Doxyfile has CREATE_SUBDIRS, which the HTML site keeps, so the
      XML files lie in two levels of directories and their refids carry
      the path ("de/d75/class_..."); Exhale names its pages after the refid
      but links them by the name alone, so the files are moved up into
      doc/xml and the path is dropped from every id and refid, which stay
      unique since the path is a hash of the name;
    - the friends that a group lists come in a <sectiondef kind="">, which
      the schema of Breathe rejects, and become a section of friends;
    - the caption of a table comes with no id, which the schema requires,
      and is given one;
    - the logo that doxygen/mainpage.md shows is copied next to the XML,
      where the page that Exhale makes of it looks for it"""
    xml = breathe_projects['SMS++']
    if not os.path.isdir(xml):
        return
    shutil.copy(os.path.join(ROOT, 'doxygen', 'SMSpp_logo_mid_noback.png'),
                xml)
    subdir = re.compile(r'\b((?:ref)?id)="d[0-9a-f]/d[0-9a-f]{2}/')
    for top, _, files in sorted(os.walk(xml), reverse=True):
        for name in files:
            if not name.endswith('.xml'):
                continue
            path = os.path.join(top, name)
            with open(path) as f:
                text = f.read()
            new = subdir.sub(r'\1="', text)
            new = new.replace('<sectiondef kind="">',
                              '<sectiondef kind="friend">')
            parts = new.split('<caption>')
            new = parts[0] + ''.join('<caption id="%s_caption%d">%s'
                                     % (name[:-4], k, part)
                                     for k, part in enumerate(parts[1:]))
            if new != text or top != xml:
                with open(os.path.join(xml, name), 'w') as f:
                    f.write(new)
                if top != xml:
                    os.remove(path)
        if top != xml and not os.listdir(top):
            os.rmdir(top)


mend_xml()

exhale_args = {
    'containmentFolder': './api',
    'rootFileName': 'library_root.rst',
    'doxygenStripFromPath': ROOT,
    'rootFileTitle': 'API Reference',
    'afterTitleDescription':
        'The classes and the files of the core library and of the modules; '
        'the same reference, with the graphs of Doxygen, is the '
        '`Doxygen site <https://smspp.gitlab.io/smspp-project/>`_.',
    'createTreeView': True,
    'exhaleExecutesDoxygen': False,
}

html_theme = 'sphinx_rtd_theme'
# the logo without background, on the grey of the sidebar, where its blue M
# stands out (on the blue of the theme it does not), and alone, since it
# spells the name of the project
html_logo = os.path.join(ROOT, 'doxygen', 'SMSpp_logo_mid_noback.png')
# the sidebar of each page holds the branch of that page alone: with the
# whole tree of the API in each of its 2000 pages the build takes about
# nine times as long (55 minutes instead of 6, measured on one core)
html_theme_options = {'navigation_depth': 3, 'collapse_navigation': True,
                      'logo_only': True,
                      'style_nav_header_background': '#343131'}

# --------------------------------------------------------------------------- #
#                              End of conf.py                                 #
# --------------------------------------------------------------------------- #
