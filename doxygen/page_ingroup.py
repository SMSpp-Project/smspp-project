#!/usr/bin/env python3
# --------------------------------------------------------------------------- #
# page_ingroup.py                                                             #
#                                                                             #
# The input filter of Doxygen that puts the pages of a header in a group of   #
# the Topics: it prints the header with a line "\ingroup <group>" after each  #
# line with "\page", so that the page goes under its module in the tree of    #
# the site instead of at its top, which an \ingroup outside the block of the  #
# page cannot do. sphinx/apiref.py writes the FILTER_PATTERNS that call it    #
# on the headers of the pages, with the group of each, into                   #
# doxygen/pages.cfg, which the second run of Doxygen reads.                   #
#                                                                             #
#     page_ingroup.py <group> <header>                                        #
# --------------------------------------------------------------------------- #

import re
import sys

group, header = sys.argv[1], sys.argv[2]
with open(header) as f:
    for line in f:
        sys.stdout.write(line)
        if re.search(r'[@\\]page\s', line):
            sys.stdout.write(' *  \\ingroup %s\n' % group)
