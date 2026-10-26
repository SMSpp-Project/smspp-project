# --------------------------------------------------------------------------- #
#                                 conf.py                                     #
#                                                                             #
# The configuration of Sphinx for the second site of the documentation of     #
# SMS++, next to the one of Doxygen and to the user manual: the pages that    #
# sphinx/pages.py writes from the Markdown files of the umbrella (read by     #
# MyST), and the API reference, whose pages sphinx/apiref.py writes from      #
# the XML that doxygen/Doxyfile writes into doc/xml and Breathe renders.      #
# The theme is that of Read the Docs. The pages job of .gitlab-ci.yml         #
# builds it with                                                              #
#                                                                             #
#   doxygen doxygen/Doxyfile                                                  #
#   python3 sphinx/pages.py                                                   #
#   sphinx-build -b html -d doc/doctrees sphinx public/sphinx                 #
# --------------------------------------------------------------------------- #

import os
import re
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
    'myst_parser',          # the pages in Markdown
    'sphinx.ext.mathjax',   # the formulae of the comments
    'sphinx_copybutton',    # a button that copies a block of code
    'sphinx_rtd_dark_mode', # a switch between the light and the dark theme
]

# the light theme first, the dark one on the switch, which the browser
# remembers
default_dark_mode = False

# a signature longer than this goes one parameter per line, instead of
# being cut at the margin or wrapped at random
maximum_signature_line_length = 110

# the prompts of the shells are not copied with the commands
copybutton_prompt_text = r'\$ |>>> '
copybutton_prompt_is_regexp = True

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# what the comments of the headers cause and the configuration cannot mend:
# a member that Doxygen lists twice (in a class and in a group) is
# declared twice, and the \verbatim blocks with formulae fail to be lexed
# as C++
suppress_warnings = ['duplicate_declaration.cpp', 'misc.highlighting_failure']

breathe_projects = {'SMS++': os.path.join(ROOT, 'doc', 'xml')}
breathe_default_project = 'SMS++'
primary_domain = 'cpp'
highlight_language = 'cpp'

# a "friend X;" that Doxygen reads as a variable of type friend
FRIEND = re.compile(r'(<memberdef )kind="variable"([^>]*>\s*)'
                    r'<type>friend</type>')

# the elements whose text keeps its lines
VERBATIM = re.compile(r'<(/?)(verbatim|preformatted|programlisting|formula)'
                      r'\b[^>]*?(/?)>')


def join_lines(text):
    """the ends of the lines inside the text of the comments made spaces,
    out of the elements whose text keeps its lines"""
    out, keep = [], 0
    for part in re.split(r'(<[^>]*>)', text):
        if part.startswith('<'):
            m = VERBATIM.match(part)
            if m and not m.group(3):
                keep += -1 if m.group(1) else 1
        elif keep == 0 and '\n' in part.strip():
            body = part.strip()
            lead = part[:len(part) - len(part.lstrip())]
            trail = part[len(part.rstrip()):]
            part = lead + re.sub(r'\s*\n\s*', ' ', body) + trail
        out.append(part)
    return ''.join(out)


def mend_xml():
    """the XML of doxygen/Doxyfile made fit for Breathe, in place (it is a
    product of the build, which is read and never published):

    - the Doxyfile has CREATE_SUBDIRS, which the HTML site keeps, so the
      XML files lie in two levels of directories and their refids carry
      the path ("de/d75/class_...") into the anchors of the pages; the
      files are moved up into doc/xml and the path is dropped from every
      id and refid, which stay unique since the path is a hash of the name
      (sphinx/apiref.py, which reads the XML before, finds the pages of
      the Doxygen site from the name alone);
    - the friends that a group lists come in a <sectiondef kind="">, which
      the schema of Breathe rejects, and become a section of friends;
    - the caption of a table comes with no id, which the schema requires,
      and is given one;
    - the text between quotes keeps the ends of the lines of the comment,
      which Breathe turns into as many paragraphs (a reference whose title
      spans two lines comes out cut in two), and they become spaces;
    - a "friend X;" without "class" is taken by Doxygen for a variable X
      of type friend, which the C++ domain of Sphinx cannot parse, and
      becomes the friend class it is"""
    xml = breathe_projects['SMS++']
    if not os.path.isdir(xml):
        return
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
            new = join_lines(new)
            new = FRIEND.sub(r'\1kind="friend"\2<type>class</type>', new)
            if new != text or top != xml:
                with open(os.path.join(xml, name), 'w') as f:
                    f.write(new)
                if top != xml:
                    os.remove(path)
        if top != xml and not os.listdir(top):
            os.rmdir(top)


mend_xml()

html_theme = 'sphinx_rtd_theme'
# the logo without background, on the grey of the sidebar, where its blue M
# stands out (on the blue of the theme it does not), and alone, since it
# spells the name of the project
html_logo = os.path.join(ROOT, 'doxygen', 'SMSpp_logo_mid_noback.png')
# the content as wide as the formulae need, which scroll in their box when
# wider still (see _static/smspp.css)
html_static_path = ['_static']
html_css_files = ['smspp.css']
# the sidebar stays in place while the page scrolls and scrolls on its own;
# it holds the modules and the groups, and the branch of the page down to
# its classes. The whole tree in each page (collapse_navigation False, each
# branch with its switch) makes the build of the core and of UCBlock, 300
# pages, take 6 minutes instead of 1.5, and 5 with the tree down to the
# headers only; the build grows with the square of the pages
html_theme_options = {'navigation_depth': 5, 'collapse_navigation': True,
                      'sticky_navigation': True, 'logo_only': True,
                      'style_nav_header_background': '#343131'}

# --------------------------------------------------------------------------- #
#                              End of conf.py                                 #
# --------------------------------------------------------------------------- #
