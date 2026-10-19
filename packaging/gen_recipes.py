#!/usr/bin/env python3
"""Writes the recipes of the channels of this directory from modules.py.

Usage: gen_recipes.py [--release X.Y.Z] [--url-base URL]

Without arguments it rewrites every recipe for the release in release.json.
With --release it first downloads the tarball of that release of the
umbrella, writes its version and its SHA-256 into release.json, and then
rewrites the recipes. --url-base replaces the address of the generic package
registry where the release tarballs live, so that a recipe can be tried on a
tarball that is not published (the recipes it writes are then not to be
committed).

The AUR and MSYS2 PKGBUILDs and the .SRCINFO files are written whole; the
other recipes are written by hand around a table of the modules, between the
lines "BEGIN of the table written by gen_recipes.py" and "END of ...", and
only that table is rewritten.
"""
import argparse
import base64
import hashlib
import json
import os
import re
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True
import modules as M  # noqa: E402

REGISTRY = ("https://gitlab.com/api/v4/projects/smspp%2Fsmspp-project/"
            "packages/generic/smspp-project")


def tarball_url(base, version):
    return f"{base}/{version}/smspp-project-{version}.tar.gz"


def sri(sha256hex):
    """The hash of Nix, i.e., sha256- and the base64 of the digest."""
    return "sha256-" + base64.b64encode(bytes.fromhex(sha256hex)).decode()


def split_url(s, width):
    """s cut after a slash into pieces of at most width characters."""
    out, cur = [], ""
    for part in re.split(r"(?<=/)", s):
        if cur and len(cur) + len(part) > width:
            out.append(cur)
            cur = ""
        cur += part
    return out + [cur]


def split_hex(s):
    """A hash in two halves."""
    return [s[:len(s) // 2], s[len(s) // 2:]]


def py_str(name, s, width=70):
    """name = s in Python, broken over lines if long."""
    if len(name) + len(s) + 5 <= 79:
        return [f"{name} = {s!r}"]
    parts = split_hex(s) if re.fullmatch(r"[0-9a-f]+", s) else \
        split_url(s, width)
    return [f"{name} = ("] + [f"    {p!r}" for p in parts] + [")"]


def write(path, text):
    path = os.path.join(HERE, path)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)
    print(f"wrote {os.path.relpath(path, HERE)}")


def replace_table(path, table, comment="#"):
    """Rewrites the lines between the BEGIN and END markers of path."""
    full = os.path.join(HERE, path)
    with open(full) as f:
        src = f.read()
    begin = f"{comment} BEGIN of the table written by gen_recipes.py\n"
    end = f"{comment} END of the table written by gen_recipes.py\n"
    i = src.index(begin) + len(begin)
    j = src.index(end, i)
    # the table keeps the indentation of the line of the END marker
    k = src.rfind("\n", 0, j) + 1
    indent = src[k:j]
    body = "".join(indent + line + "\n" if line else "\n"
                   for line in table.split("\n"))
    src = src[:i] + body + indent + src[j:]
    with open(full, "w") as f:
        f.write(src)
    print(f"wrote the table of {path}")


# ----- the externals of each channel ------------------------------------- #
# abstract name -> (build dependency, run dependency) of the channel, or
# None where the channel has no package of it; a module whose required
# externals are not all here is left out of the channel

ARCH = {
    "boost": ("boost", "boost"),
    "eigen": ("eigen", "eigen"),
    "netcdf-cxx4": ("netcdf-cxx", "netcdf-cxx"),
    "openmp": None,                  # libgomp, in gcc-libs
    "stopt": ("stopt", "stopt"),     # AUR, aur/stopt in this directory
    "libsvm": ("libsvm", "libsvm"),
    "liblinear": ("liblinear", "liblinear"),   # AUR
    "cadical": ("cadical", "cadical"),
    "minisat": ("minisat", "minisat"),
    "highs": ("highs", "highs"),
    "scip": ("scip", "scip"),
    # the static library of the osqp package, which MILPSolver prefers, is
    # made of slim LTO objects, which a build without LTO cannot link
    "osqp": None,
    "lemon": ("coin-or-lemon", "coin-or-lemon"),
}


def disabled_finds(table):
    """The CMake names of the optional externals the channel has not."""
    return [M.CMAKE_NAME[e] for e in M.CMAKE_NAME if not table.get(e)]


def channel_modules(table):
    """The modules whose required externals the channel has."""
    ok = {m[1] for m in M.MODULES if all(table.get(r) for r in m[3])}
    needs = {m[1]: m[2] for m in M.MODULES}
    changed = True
    while changed:
        drop = {a for a in ok if not all(n in ok for n in needs[a])}
        ok -= drop
        changed = bool(drop)
    return [m[1] for m in M.MODULES if m[1] in ok]


def channel_tools(mods):
    return [t for t in M.TOOLS if all(n in mods for n in t[2])]


# ----- AUR ---------------------------------------------------------------- #

def bash_array(name, items, indent="  "):
    """name=(items) on one line if it fits in 79 columns, else one per line."""
    one = f"{indent}{name}=({' '.join(items)})"
    if len(one) <= 79:
        return [one]
    return [f"{indent}{name}=("] + [f"{indent}  {i}" for i in items] + \
        [f"{indent})"]


def arch_pkg(acr):
    return "libsmspp" if acr == "core" else f"libsmspp-{acr}"


def gen_aur(rel, base):
    mods = channel_modules(ARCH)
    tools = channel_tools(mods)
    info = {m[1]: m for m in M.MODULES}
    off = [m[0] for m in M.MODULES if m[1] not in mods]
    helpers = {h[0]: [] for h in M.HELPERS}
    for macr, exe, tacr in M.HELPERS:
        helpers[macr].append((exe, tacr))

    makedeps = ["cmake", "help2man"]
    for acr in mods:
        for e in info[acr][3] + info[acr][4]:
            if ARCH.get(e) and ARCH[e][0] not in makedeps:
                makedeps.append(ARCH[e][0])

    pkgnames = [arch_pkg(a) for a in mods] + \
        [f"smspp-{t[0]}" for t in tools] + ["smspp-project"]
    v = rel["version"]
    ff = M.FASTFLOW
    L = []
    w = L.append
    w(f"# Maintainer: {M.MAINTAINER}")
    w("#")
    w("# Written by packaging/gen_recipes.py of the SMS++ umbrella from its")
    w("# packaging/modules.py: edit those, not this file.")
    w("")
    w("pkgbase=smspp-project")
    # the tarball of a release is in the generic package registry
    if base == REGISTRY:
        w("_gitlab=https://gitlab.com/api/v4/projects")
        w("_registry=$_gitlab/smspp%2Fsmspp-project/packages/generic/"
          "smspp-project")
    else:
        w(f"_registry={base}")
    w("# FastFlow, which the core fetches at configure time, at the commit")
    w("# that the other packages of the project pin")
    w(f"_ff={ff['commit']}")
    w("_ffurl=https://github.com/fastflow/fastflow/archive")
    w("pkgname=(")
    for p in pkgnames:
        w(f"  {p}")
    w(")")
    w(f"pkgver={v}")
    w("pkgrel=1")
    w("_tarball=smspp-project-$pkgver.tar.gz")
    w(f"pkgdesc=\"{M.SUMMARY}\"")
    w("arch=('x86_64' 'aarch64')")
    w(f"url=\"{M.HOMEPAGE}\"")
    w("license=('LGPL-3.0-only')")
    w("makedepends=(")
    for d in makedeps:
        w(f"  {d}")
    w(")")
    w("# link-time optimization of a code base this large is a memory sink")
    w("options=('!lto')")
    w("source=(")
    w("  \"$_tarball::$_registry/$pkgver/$_tarball\"")
    w("  \"fastflow-$_ff.tar.gz::$_ffurl/$_ff.tar.gz\"")
    w(")")
    w("# the tarball of the release has no top directory")
    w("noextract=(\"$_tarball\")")
    w("sha256sums=(")
    w(f"  '{rel['sha256']}'")
    w(f"  '{ff['sha256']}'")
    w(")")
    w("")
    w("prepare() {")
    w("  mkdir -p smspp-project-$pkgver")
    w("  bsdtar -xf $_tarball -C smspp-project-$pkgver")
    w("  # the settings of the umbrella for its developers give way to these")
    w("  rm -f smspp-project-$pkgver/CMakeSettings.txt")
    w("}")
    w("")
    w("build() {")
    w("  # the build type None of Arch defines no NDEBUG")
    w("  CXXFLAGS+=\" -DNDEBUG\"")
    w("  # CPLEX, Gurobi and MOSEK are not redistributable, the C interface")
    w("  # of Clarabel and the other QP solvers are in no package; FastFlow,")
    w("  # which the core fetches at configure time, comes as a source; the")
    w("  # header of CaDiCaL is in a directory of its own")
    w("  cmake -S smspp-project-$pkgver -B build \\")
    w("    -DCMAKE_BUILD_TYPE=None \\")
    w("    -DCMAKE_INSTALL_PREFIX=/usr \\")
    w("    -DBUILD_SHARED_LIBS=ON \\")
    w("    -DBUILD_tests=OFF \\")
    w("    -DBUILD_TESTING=OFF \\")
    w("    -DBUILD_experiments=OFF \\")
    for d in off:
        w(f"    -DBUILD_{d}=OFF \\")
    w("    -DCMAKE_SKIP_INSTALL_RPATH=ON \\")
    w("    -DFETCHCONTENT_SOURCE_DIR_FASTFLOW=\"$srcdir/fastflow-$_ff\" \\")
    w("    -DFETCHCONTENT_FULLY_DISCONNECTED=ON \\")
    for p in M.NEVER + disabled_finds(ARCH):
        w(f"    -DCMAKE_DISABLE_FIND_PACKAGE_{p}=ON \\")
    for e, var in M.ROOT_VAR.items():
        if ARCH.get(e):
            w(f"    -D{var}=/usr \\")
    for var in M.NO_SOURCES:
        w(f"    -D{var}=OFF \\")
    w("    -DCADICAL_INCLUDE_DIR=/usr/include/cadical \\")
    w("    -DUSE_MPI=OFF")
    w("  cmake --build build")
    w("}")
    w("")
    w("# the one build of the umbrella is installed one module at a time,")
    w("# each in its package, and so are the tools; the install goes through")
    w("# DESTDIR, since the link that carries the old name of a tool is made")
    w("# of the prefix of the configure")
    for acr in mods:
        d, _, needs, req, opt, desc = info[acr]
        deps = [arch_pkg(n) for n in needs]
        for e in req + opt:
            if ARCH.get(e) and ARCH[e][1] not in deps:
                deps.append(ARCH[e][1])
        w("")
        w(f"package_{arch_pkg(acr)}() {{")
        w(f"  pkgdesc=\"SMS++: {desc}\"")
        if deps:
            L.extend(bash_array("depends", deps))
        w(f"  DESTDIR=\"$pkgdir\" cmake --install build/{d}")
        for exe, _ in helpers.get(acr, []):
            w(f"  rm \"$pkgdir/usr/bin/{exe}\"")
        if helpers.get(acr):
            w("  rmdir --ignore-fail-on-non-empty \"$pkgdir/usr/bin\"")
        w("}")
    for tacr, dirs, needs, desc in tools:
        deps = [arch_pkg(n) for n in M.closure(needs)]
        w("")
        w(f"package_smspp-{tacr}() {{")
        w(f"  pkgdesc=\"SMS++: {desc}\"")
        L.extend(bash_array("depends", deps))
        for t in dirs:
            w(f"  DESTDIR=\"$pkgdir\" cmake --install build/tools/{t}")
        for macr, exe, ta in M.HELPERS:
            if ta == tacr and macr in mods:
                w(f"  DESTDIR=\"$srcdir/helper-{exe}\" \\")
                w(f"    cmake --install build/{info[macr][0]}")
                w(f"  install -Dm755 -t \"$pkgdir/usr/bin\" \\")
                w(f"    \"$srcdir/helper-{exe}/usr/bin/{exe}\"")
        w("}")
    w("")
    w("package_smspp-project() {")
    w("  pkgdesc=\"SMS++: every module, with its headers, and every tool\"")
    w("  arch=('any')")
    w("  depends=(")
    for p in pkgnames[:-1]:
        w(f"    {p}")
    w("  )")
    w("}")
    write("aur/smspp-project/PKGBUILD", "\n".join(L) + "\n")
    write("aur/smspp-project/.SRCINFO",
          srcinfo("smspp-project", pkgnames, v, "1", M.SUMMARY,
                  ["x86_64", "aarch64"], M.HOMEPAGE, ["LGPL-3.0-only"],
                  makedeps, ["!lto"],
                  [f"smspp-project-{v}.tar.gz::{base}/{v}/"
                   f"smspp-project-{v}.tar.gz",
                   f"fastflow-{ff['commit']}.tar.gz::{ff['url']}"],
                  [f"smspp-project-{v}.tar.gz"],
                  [rel["sha256"], ff["sha256"]],
                  {**{arch_pkg(a): pkg_fields(a, info, ARCH)
                      for a in mods},
                   **{f"smspp-{t[0]}":
                      {"pkgdesc": f"SMS++: {t[3]}",
                       "depends": [arch_pkg(n) for n in M.closure(t[2])]}
                      for t in tools},
                   "smspp-project":
                      {"pkgdesc": "SMS++: every module, with its headers, "
                                  "and every tool",
                       "arch": ["any"], "depends": pkgnames[:-1]}}))


def pkg_fields(acr, info, table):
    d, _, needs, req, opt, desc = info[acr]
    deps = [arch_pkg(n) for n in needs]
    for e in req + opt:
        if table.get(e) and table[e][1] not in deps:
            deps.append(table[e][1])
    out = {"pkgdesc": f"SMS++: {desc}"}
    if deps:
        out["depends"] = deps
    return out


def srcinfo(pkgbase, pkgnames, ver, rel, desc, arches, url, licenses,
            makedeps, options, sources, noextract, sums, per_pkg,
            depends=()):
    """The .SRCINFO that makepkg --printsrcinfo writes for the PKGBUILD."""
    L = [f"pkgbase = {pkgbase}", f"\tpkgdesc = {desc}",
         f"\tpkgver = {ver}", f"\tpkgrel = {rel}", f"\turl = {url}"]
    L += [f"\tarch = {a}" for a in arches]
    L += [f"\tlicense = {x}" for x in licenses]
    L += [f"\tmakedepends = {d}" for d in makedeps]
    L += [f"\tdepends = {d}" for d in depends]
    L += [f"\tnoextract = {x}" for x in noextract]
    L += [f"\toptions = {o}" for o in options]
    L += [f"\tsource = {s}" for s in sources]
    L += [f"\tsha256sums = {s}" for s in sums]
    L.append("")
    for p in pkgnames:
        L.append(f"pkgname = {p}")
        f = per_pkg.get(p, {})
        if "pkgdesc" in f:
            L.append(f"\tpkgdesc = {f['pkgdesc']}")
        for a in f.get("arch", []):
            L.append(f"\tarch = {a}")
        for d in f.get("depends", []):
            L.append(f"\tdepends = {d}")
        L.append("")
    return "\n".join(L)


def gen_aur_stopt():
    s = M.STOPT
    L = f"""# Maintainer: {M.MAINTAINER}
#
# Written by packaging/gen_recipes.py of the SMS++ umbrella from its
# packaging/modules.py: edit those, not this file.

pkgname=stopt
pkgver={s['version']}
pkgrel=1
pkgdesc="Library for stochastic optimization by dynamic programming and SDDP"
arch=('x86_64' 'aarch64')
url="https://gitlab.com/stochastic-control/StOpt"
license=('LGPL-3.0-only')
depends=(boost boost-libs eigen zlib bzip2)
makedepends=(cmake)
_tarball=StOpt-v$pkgver.tar.gz
source=("$_tarball::$url/-/archive/v$pkgver/$_tarball")
sha256sums=('{s['sha256']}')

build() {{
  # the geners serialization library travels inside StOpt and is built
  # with it; the SDDP and DP CUTS switches build tests, not the library
  cmake -S StOpt-v$pkgver -B build \\
    -DCMAKE_BUILD_TYPE=None \\
    -DCMAKE_INSTALL_PREFIX=/usr \\
    -DBUILD_SHARED_LIBS=ON \\
    -DBUILD_PYTHON=OFF \\
    -DBUILD_TEST=OFF \\
    -DBUILD_SDDP=OFF \\
    -DBUILD_DPCUTS=OFF \\
    -DBUILD_MPI=OFF \\
    -DBUILD_SYSTEM_INSTALL=ON
  cmake --build build
}}

package() {{
  DESTDIR="$pkgdir" cmake --install build
}}
"""
    write("aur/stopt/PKGBUILD", L)
    url = s['url']
    write("aur/stopt/.SRCINFO", srcinfo(
        "stopt", ["stopt"], s["version"], "1",
        "Library for stochastic optimization by dynamic programming and SDDP",
        ["x86_64", "aarch64"], "https://gitlab.com/stochastic-control/StOpt",
        ["LGPL-3.0-only"], ["cmake"], [],
        [f"StOpt-v{s['version']}.tar.gz::{url}"], [], [s["sha256"]], {},
        depends=["boost", "boost-libs", "eigen", "zlib", "bzip2"]))


# ----- MSYS2 -------------------------------------------------------------- #
# the packages are mingw-w64-<arch>-<name>; StOpt, LEMON, CaDiCaL, MiniSat,
# libsvm, liblinear and SCIP are not in MSYS2, so SDDPBlock, InvestmentBlock
# and MCFLemonSolver are left out and SATBlock and SVMBlock come without the
# solvers they wrap

MSYS2 = {
    "boost": ("boost", "boost"),
    "eigen": ("eigen3", "eigen3"),
    "netcdf-cxx4": ("netcdf-cxx", "netcdf-cxx"),
    "highs": ("highs", "highs"),
    "openmp": None,
}


def gen_msys2(rel, base):
    mods = channel_modules(MSYS2)
    off = [m[0] for m in M.MODULES if m[1] not in mods]
    info = {m[1]: m for m in M.MODULES}
    deps = []
    for acr in mods:
        for e in info[acr][3] + info[acr][4]:
            if MSYS2.get(e) and MSYS2[e][1] not in deps:
                deps.append(MSYS2[e][1])
    ff = M.FASTFLOW
    L = []
    w = L.append
    w(f"# Maintainer: {M.MAINTAINER}")
    w("#")
    w("# Written by packaging/gen_recipes.py of the SMS++ umbrella from its")
    w("# packaging/modules.py: edit those, not this file.")
    w("")
    w("_realname=smspp")
    w("pkgbase=mingw-w64-${_realname}")
    w("pkgname=(\"${MINGW_PACKAGE_PREFIX}-${_realname}\")")
    w(f"pkgver={rel['version']}")
    w("pkgrel=1")
    w("pkgdesc=\"Framework for block-structured optimization problems "
      "(mingw-w64)\"")
    w("arch=('any')")
    w("mingw_arch=('ucrt64' 'clang64' 'clangarm64')")
    w(f"url=\"{M.HOMEPAGE}\"")
    w("msys2_references=(")
    w("  'gitlab: smspp/smspp-project'")
    w(")")
    w("license=('spdx:LGPL-3.0-only')")
    w("depends=(")
    for d in deps:
        if d != "eigen3":
            w(f"  \"${{MINGW_PACKAGE_PREFIX}}-{d}\"")
    w(")")
    w("makedepends=(")
    w("  \"${MINGW_PACKAGE_PREFIX}-cc\"")
    w("  \"${MINGW_PACKAGE_PREFIX}-cmake\"")
    w("  \"${MINGW_PACKAGE_PREFIX}-ninja\"")
    w("  \"${MINGW_PACKAGE_PREFIX}-eigen3\"")
    w(")")
    w("# FastFlow, which the core fetches at configure time, at the commit")
    w("# that the other packages of the project pin")
    w(f"_ff={ff['commit']}")
    if base == REGISTRY:
        w("_gitlab=https://gitlab.com/api/v4/projects")
        w("_registry=$_gitlab/smspp%2Fsmspp-project/packages/generic/"
          "smspp-project")
    else:
        w(f"_registry={base}")
    w("_ffurl=https://github.com/fastflow/fastflow/archive")
    w("_tarball=smspp-project-$pkgver.tar.gz")
    w("source=(")
    w("  \"$_tarball::$_registry/$pkgver/$_tarball\"")
    w("  \"fastflow-$_ff.tar.gz::$_ffurl/$_ff.tar.gz\"")
    w(")")
    w("# the tarball of the release has no top directory")
    w("noextract=(\"$_tarball\")")
    w("sha256sums=(")
    w(f"  '{rel['sha256']}'")
    w(f"  '{ff['sha256']}'")
    w(")")
    w("")
    w("prepare() {")
    w("  mkdir -p smspp-project-$pkgver")
    w("  bsdtar -xf $_tarball -C smspp-project-$pkgver")
    w("  # the settings of the umbrella for its developers give way to these")
    w("  rm -f smspp-project-$pkgver/CMakeSettings.txt")
    w("}")
    w("")
    w("build() {")
    w("  # the modules whose dependencies MSYS2 does not carry are left out;")
    w("  # CPLEX, Gurobi and MOSEK are not redistributable")
    w("  MSYS2_ARG_CONV_EXCL=\"-DCMAKE_INSTALL_PREFIX=\" \\")
    w("    cmake -S smspp-project-$pkgver -B \"build-${MSYSTEM}\" -G Ninja \\")
    w("      -DCMAKE_BUILD_TYPE=Release \\")
    w("      -DCMAKE_INSTALL_PREFIX=\"${MINGW_PREFIX}\" \\")
    w("      -DBUILD_SHARED_LIBS=ON \\")
    w("      -DBUILD_tests=OFF \\")
    w("      -DBUILD_TESTING=OFF \\")
    w("      -DBUILD_experiments=OFF \\")
    for d in off:
        w(f"      -DBUILD_{d}=OFF \\")
    w("      -DUSE_MPI=OFF \\")
    w("      -DFETCHCONTENT_SOURCE_DIR_FASTFLOW=\"$srcdir/fastflow-$_ff\" \\")
    w("      -DFETCHCONTENT_FULLY_DISCONNECTED=ON \\")
    for p in M.NEVER + disabled_finds(MSYS2):
        w(f"      -DCMAKE_DISABLE_FIND_PACKAGE_{p}=ON \\")
    for e, var in M.ROOT_VAR.items():
        if MSYS2.get(e):
            w(f"      -D{var}=\"${{MINGW_PREFIX}}\" \\")
    for var in M.NO_SOURCES:
        w(f"      -D{var}=OFF \\")
    # the Find module of netCDF-C++ looks on Windows for the name of the
    # library of vcpkg alone, while MSYS2 names it netcdf_c++4
    for var in ("netCDFCxx_LIBRARY", "netCDFCxx_LIBRARY_DEBUG"):
        w(f"      -D{var}=\"${{MINGW_PREFIX}}/lib/libnetcdf_c++4.dll.a\" \\")
    L[-1] = L[-1][:-2]
    w("  cmake --build \"build-${MSYSTEM}\"")
    w("}")
    w("")
    w("package() {")
    w("  DESTDIR=\"${pkgdir}\" cmake --install \"build-${MSYSTEM}\"")
    w("  install -Dm644 smspp-project-$pkgver/LICENSE \\")
    w("    \"${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE\"")
    w("}")
    write("msys2/mingw-w64-smspp/PKGBUILD", "\n".join(L) + "\n")


# ----- Nix ---------------------------------------------------------------- #

def nix_list(items, indent=""):
    """A list of strings in Nix, one per line if it does not fit."""
    one = "[ " + " ".join(f'"{i}"' for i in items) + " ]" if items \
        else "[ ]"
    if len(indent) + len(one) <= 66:
        return one
    return "[\n" + "".join(f'{indent}  "{i}"\n' for i in items) + \
        indent + "]"


def nix_str(name, s, indent=""):
    """name = s; in Nix, cut after a slash if long."""
    if len(indent) + len(name) + len(s) + 6 <= 79:
        return [f'{name} = "{s}";']
    parts = split_url(s, 66 - len(indent))
    return [f'{name} =', f'  "{parts[0]}"'] + \
        [f'  + "{p}"' for p in parts[1:-1]] + [f'  + "{parts[-1]}";']


def gen_nix(rel, base):
    ff = M.FASTFLOW
    L = [f'version = "{rel["version"]}";',
         f'hash = "{sri(rel["sha256"])}";'] + \
        nix_str("registry", base, "  ") + \
        [f'fastflowCommit = "{ff["commit"]}";',
         f'fastflowHash = "{sri(ff["sha256"])}";',
         "# the modules: directory, acronym, the modules it needs, its",
         "# required and optional externals",
         "table = ["]
    for d, acr, needs, req, opt, _ in M.MODULES:
        L += ["  {",
              f'    dir = "{d}";',
              f'    acr = "{acr}";',
              f"    needs = {nix_list(needs, '    ')};",
              f"    req = {nix_list(req, '    ')};",
              f"    opt = {nix_list(opt, '    ')};",
              "  }"]
    L.append("];")
    L.append("# the solvers that are never packaged")
    L.append(f"never = {nix_list(M.NEVER)};")
    L.append("# the CMake names of the optional externals")
    L.append("cmakeName = {")
    for e, n in M.CMAKE_NAME.items():
        L.append(f'  {e} = "{n}";')
    L.append("};")
    L.append("# the variables that tell the CMake of the modules where an")
    L.append("# external is, and the sources it is not to compile in")
    L.append(f"rootVars = {nix_list(list(M.ROOT_VAR.values()))};")
    L.append(f"noSources = {nix_list(M.NO_SOURCES)};")
    replace_table("nix/package.nix", "\n".join(L))
    replace_table("nix/stopt.nix",
                  f'version = "{M.STOPT["version"]}";\n'
                  f'hash = "{sri(M.STOPT["sha256"])}";')


# ----- Python tables (Spack, Conan) ---------------------------------------- #

def py_modules():
    L = ["_MODULES = ["]
    for d, acr, needs, req, opt, desc in M.MODULES:
        head = f"    ({d!r}, {acr!r}, {needs!r}, {req!r},"
        if len(head) + len(repr(opt)) + 2 <= 79:
            L.append(f"{head} {opt!r},")
        else:
            L.append(head)
            items = [repr(o) for o in opt]
            lines, cur = [], "     ["
            for k, it in enumerate(items):
                piece = it + (", " if k < len(items) - 1 else "],")
                if len(cur) + len(piece.rstrip()) > 79:
                    lines.append(cur.rstrip())
                    cur = "      "
                cur += piece
            L.extend(lines + [cur])
        L.append(f"     {desc!r}),")
    L.append("]")
    L.append(f"_NEVER = {M.NEVER!r}")
    L.append(f"_NO_SOURCES = {M.NO_SOURCES!r}")
    for name, d in (("_CMAKE_NAME", M.CMAKE_NAME),
                    ("_ROOT_VAR", M.ROOT_VAR)):
        L.append(name + " = {")
        for e, n in d.items():
            L.append(f"    {e!r}: {n!r},")
        L.append("}")
    return L


def gen_spack(rel, base):
    ff = M.FASTFLOW
    L = (py_str("_RELEASE", rel["version"])
         + py_str("_SHA256", rel["sha256"])
         + py_str("_URL", tarball_url(base, rel["version"]))
         + py_str("_FASTFLOW_URL", ff["url"])
         + py_str("_FASTFLOW_SHA256", ff["sha256"])
         + py_modules())
    replace_table("spack/spack_repo/smspp/packages/smspp/package.py",
                  "\n".join(L))
    a, b = split_hex(M.STOPT["sha256"])
    replace_table("spack/spack_repo/smspp/packages/stopt/package.py",
                  f"version(\n    {M.STOPT['version']!r},\n"
                  f"    sha256={a!r}\n    {b!r},\n)")


# ----- Conan --------------------------------------------------------------- #

def gen_conan(rel, base):
    ff = M.FASTFLOW
    replace_table("conan/smspp/conanfile.py", "\n".join(py_modules()))
    write("conan/smspp/conandata.yml",
          "# written by packaging/gen_recipes.py of the SMS++ umbrella\n"
          "sources:\n"
          f'  "{rel["version"]}":\n'
          "    smspp:\n"
          f'      url: "{tarball_url(base, rel["version"])}"\n'
          f'      sha256: "{rel["sha256"]}"\n'
          "    fastflow:\n"
          f'      url: "{ff["url"]}"\n'
          f'      sha256: "{ff["sha256"]}"\n')
    for name, d in (("stopt", M.STOPT), ("netcdf-cxx4", M.NETCDF_CXX4)):
        write(f"conan/{name}/conandata.yml",
              "# written by packaging/gen_recipes.py of the SMS++ umbrella\n"
              "sources:\n"
              f'  "{d["version"]}":\n'
              f'    url: "{d["url"]}"\n'
              f'    sha256: "{d["sha256"]}"\n')


# ----- xmake --------------------------------------------------------------- #

def lua_list(items):
    return "{" + ", ".join(f'"{i}"' for i in items) + "}"


def lua_str(s, indent):
    """s in Lua, cut after a slash and joined by .. if long."""
    if len(indent) + len(s) + 2 <= 79:
        return [f'{indent}"{s}"']
    parts = split_url(s, 74 - len(indent))
    return [f'{indent}"{parts[0]}"'] + \
        [f'{indent}.. "{p}"' for p in parts[1:]]


def lua_versions(version, sha256):
    return [f'add_versions("{version}",', f'    "{sha256}")']


def gen_xmake(rel, base):
    ff = M.FASTFLOW
    L = ["add_urls("] + \
        lua_str(f"{base}/$(version)/smspp-project-$(version).tar.gz",
                "    ") + [")"]
    L += lua_versions(rel["version"], rel["sha256"])
    L += ["-- FastFlow, which the core fetches at configure time",
          'add_resources("*", "fastflow",']
    L += lua_str(ff["url"], "              ")
    L[-1] += ","
    L += [f'    "{ff["sha256"]}")', "",
          "-- the modules: directory, acronym, library, the modules it needs,",
          "-- its required and optional externals",
          "local modules = {"]
    for d, acr, needs, req, opt, desc in M.MODULES:
        L.append(f'    {{dir = "{d}", acr = "{acr}",')
        L.append(f'     lib = "{d}", needs = {lua_list(needs)},')
        L.append(f"     req = {lua_list(req)},")
        o = f"     opt = {lua_list(opt)},"
        if len(o) + 4 > 79:
            half = len(opt) // 2
            o = (f"     opt = {lua_list(opt[:half])[:-1]},\n"
                 f"            {lua_list(opt[half:])[1:]},")
        L.append(o)
        L.append(f'     desc = "{desc}"}},')
    L.append("}")
    L.append("-- the solvers that are never packaged, the CMake names of the")
    L.append("-- optional externals, the variables that tell where an")
    L.append("-- external is and the sources not to compile in")
    half = len(M.NEVER) // 2
    L.append(f"local never = {lua_list(M.NEVER[:half])[:-1]},")
    L.append(f"               {lua_list(M.NEVER[half:])[1:]}")
    L.append("local cmake_name = {")
    for e, n in M.CMAKE_NAME.items():
        L.append(f'    {e} = "{n}",')
    L.append("}")
    L.append("local root_var = {")
    for e, n in M.ROOT_VAR.items():
        L.append(f'    {e} = "{n}",')
    L.append("}")
    L.append(f"local no_sources = {lua_list(M.NO_SOURCES)}")
    replace_table("xmake/packages/s/smspp/xmake.lua", "\n".join(L), "--")
    for name, d in (("s/stopt", M.STOPT), ("n/netcdf-cxx4", M.NETCDF_CXX4)):
        replace_table(f"xmake/packages/{name}/xmake.lua",
                      "\n".join(lua_versions(d["version"], d["sha256"])),
                      "--")


# ----- main --------------------------------------------------------------- #

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--release", help="version of the umbrella to package")
    ap.add_argument("--url-base", default=REGISTRY,
                    help="where the release tarballs are")
    a = ap.parse_args()
    relpath = os.path.join(HERE, "release.json")
    if a.release:
        url = tarball_url(a.url_base, a.release)
        print(f"downloading {url}")
        h = hashlib.sha256()
        with urllib.request.urlopen(url) as r:
            for chunk in iter(lambda: r.read(1 << 20), b""):
                h.update(chunk)
        with open(relpath, "w") as f:
            json.dump({"version": a.release, "sha256": h.hexdigest()}, f,
                      indent=2)
            f.write("\n")
    with open(relpath) as f:
        rel = json.load(f)
    gen_aur(rel, a.url_base)
    gen_aur_stopt()
    gen_msys2(rel, a.url_base)
    gen_nix(rel, a.url_base)
    gen_spack(rel, a.url_base)
    gen_conan(rel, a.url_base)
    gen_xmake(rel, a.url_base)


if __name__ == "__main__":
    main()
