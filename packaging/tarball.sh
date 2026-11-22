#!/bin/sh

# --------------------------------------------------------------------------- #
#                               tarball.sh                                    #
#                                                                             #
# Makes the tarball of a release of the umbrella out of the checkout it is    #
# run in, the same that the release job publishes and that the recipes of     #
# packaging/ and of the other channels build: the submodules at their         #
# gitlinks, each VERSION.txt filled with the version of its repository (the   #
# tarball carries no .git), FastFlow and RECORD in third-party/, and no       #
# data, wiki or git metadata.                                                 #
#                                                                             #
# Usage: packaging/tarball.sh VERSION DIR                                     #
#                                                                             #
# run from the root of the umbrella; it writes                                #
# DIR/smspp-project-VERSION.tar.gz, where DIR is a directory of the root      #
# (release-artifacts in the CI), which the tarball leaves out. The release    #
# job runs it with the tag, the packaging jobs with the version of            #
# packaging/release.json.                                                     #
#                                                                             #
# GNU tar, git and python3 are needed.                                        #
# --------------------------------------------------------------------------- #

set -e

if [ $# -ne 2 ]; then
    echo "Usage: packaging/tarball.sh VERSION DIR" >&2
    exit 2
fi
version=$1
out=${2%/}

git submodule sync --recursive
git submodule update --init --recursive

# the tarball carries no .git, so every VERSION.txt gets the version of its
# repository here, as git archive would do from the export-subst
echo "$version" > VERSION.txt
git submodule foreach --quiet --recursive \
    'v=$(git describe --tags --abbrev=0 2>/dev/null) &&
     echo "$v" > VERSION.txt || true'

# FastFlow and RECORD, with their licenses, at the commits of
# packaging/modules.py: a build of the tarball fetches nothing
python3 packaging/third_party.py third-party

mkdir -p "$out"
tar --exclude="$out" --exclude='*/.*' --exclude='UCBlock/data' \
    --exclude='UCBlock/tools/mathpower2netCDF' --exclude='.git*' \
    --exclude='wiki' --exclude='org-profile' \
    --exclude='CapacitatedFacilityLocationBlock/data' \
    --exclude='MCFBlock/data' \
    -czf "$out/smspp-project-$version.tar.gz" .
ls -l "$out/smspp-project-$version.tar.gz"

# --------------------------------------------------------------------------- #
#                           END of tarball.sh                                 #
# --------------------------------------------------------------------------- #
