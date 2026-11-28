#!/usr/bin/env python3
"""Puts FastFlow, RECORD, PBeFS and ADS, at the commits of modules.py, in a
directory.

Usage: third_party.py DIR

Writes DIR/fastflow, the tree of FastFlow with its LICENSE.MIT,
DIR/record, the source file of RECORD and its LICENSE, and DIR/pbefs and
DIR/ads, the trees of PBeFS and ADS without what a build does not use, each
checked against its SHA-256; the commits of PBeFS and ADS have to be those
that BranchAndXSolver and PBeFS pin. The release tarball carries them in
third-party/, where the CMakeLists.txt of the umbrella finds them, so that
a build of the tarball fetches nothing.
"""
import hashlib
import io
import os
import re
import shutil
import sys
import tarfile
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True

from modules import ADS, FASTFLOW, PBEFS, RECORD  # noqa: E402


def fetch(url, sha256):
    with urllib.request.urlopen(url) as r:
        data = r.read()
    got = hashlib.sha256(data).hexdigest()
    if got != sha256:
        sys.exit("third_party.py: %s has SHA-256 %s, not %s" %
                 (url, got, sha256))
    return data


def tree(dep, name, out):
    """The archive of the commit of dep, whose only top directory is
    <name>-<commit>, in out/<name lowercase> without dep["drop"]."""
    data = fetch(dep["url"], dep["sha256"])
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
        tar.extractall(out, filter="data")
    dst = os.path.join(out, name.lower())
    shutil.rmtree(dst, ignore_errors=True)
    os.rename(os.path.join(out, name + "-" + dep["commit"]), dst)
    for d in dep.get("drop", []):
        shutil.rmtree(os.path.join(dst, d), ignore_errors=True)
    return dst


def pin(path, name):
    """The commit that the CMakeLists.txt at path pins by name, if any."""
    if not os.path.exists(path):
        return None
    with open(path) as f:
        m = re.search(name + r"\s+([0-9a-f]{40})", f.read())
    return m.group(1) if m else None


def same(what, commit, pinned, where):
    if pinned is not None and pinned != commit:
        sys.exit("third_party.py: modules.py has %s at %s, %s pins %s" %
                 (what, commit, where, pinned))


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)

    # FastFlow: the archive of the commit, whose only top directory is
    # fastflow-<commit>, renamed fastflow
    data = fetch(FASTFLOW["url"], FASTFLOW["sha256"])
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
        tar.extractall(out, filter="data")
    dst = os.path.join(out, "fastflow")
    shutil.rmtree(dst, ignore_errors=True)
    os.rename(os.path.join(out, "fastflow-" + FASTFLOW["commit"]), dst)

    # PBeFS, at the commit that BranchAndXSolver pins, and ADS, at the one
    # that PBeFS pins
    bxs = os.path.join(HERE, "..", "BranchAndXSolver", "CMakeLists.txt")
    same("PBeFS", PBEFS["commit"], pin(bxs, "BXS_PBEFS_COMMIT"),
         "BranchAndXSolver")
    pbefs = tree(PBEFS, "PBeFS", out)
    same("ADS", ADS["commit"],
         pin(os.path.join(pbefs, "CMakeLists.txt"), "PBEFS_ADS_COMMIT"),
         "PBeFS")
    tree(ADS, "ADS", out)

    # RECORD: its source file and its LICENSE
    for name, sha256 in RECORD["files"].items():
        path = os.path.join(out, "record", name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(fetch(RECORD["url"] + name, sha256))


if __name__ == "__main__":
    main()
