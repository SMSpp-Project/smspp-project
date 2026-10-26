#!/usr/bin/env python3
"""Puts FastFlow and RECORD, at the commits of modules.py, in a directory.

Usage: third_party.py DIR

Writes DIR/fastflow, the tree of FastFlow with its LICENSE.MIT, and
DIR/record, the source file of RECORD and its LICENSE, each checked against
its SHA-256. The release tarball carries them in third-party/, where the
CMakeLists.txt of the umbrella finds them, so that a build of the tarball
fetches nothing.
"""
import hashlib
import io
import os
import shutil
import sys
import tarfile
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True

from modules import FASTFLOW, RECORD  # noqa: E402


def fetch(url, sha256):
    with urllib.request.urlopen(url) as r:
        data = r.read()
    got = hashlib.sha256(data).hexdigest()
    if got != sha256:
        sys.exit("third_party.py: %s has SHA-256 %s, not %s" %
                 (url, got, sha256))
    return data


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

    # RECORD: its source file and its LICENSE
    for name, sha256 in RECORD["files"].items():
        path = os.path.join(out, "record", name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(fetch(RECORD["url"] + name, sha256))


if __name__ == "__main__":
    main()
