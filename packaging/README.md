# Recipes of SMS++ for Conan, Spack, Nix, the AUR, MSYS2 and xmake

This directory holds the recipes of the channels that distribute SMS++
besides the four with a repository of their own (the PPA, whose packaging is
[smspp/ppa](https://gitlab.com/smspp/ppa), the
[vcpkg registry](https://gitlab.com/smspp/vcpkg-registry), the Homebrew tap
and the conda-forge feedstock). They follow the design of those four: a
recipe builds the tarball of a release of the umbrella, which carries the
submodules with the version of each, and a module is named by the acronym of
its vcpkg feature and of its Debian package (`ucblock`, `milp`, `sddp`, and
so on). The packages carry the solvers that can be redistributed (HiGHS, SCIP
where the channel has it, and the QP solvers that the channel carries), and
never CPLEX, Gurobi or MOSEK.

    modules.py        the table of the modules, of the tools and of the
                      external dependencies, out of which every recipe is
                      written
    gen_recipes.py    the generator of the recipes from modules.py
    release.json      the version and the SHA-256 of the tarball that the
                      recipes build
    conan/            smspp, with netcdf-cxx4 and stopt, which Conan Center
                      does not carry
    spack/            a Spack repository (namespace smspp) with smspp, stopt
                      and highs, which the builtin one does not carry
    nix/              package.nix and stopt.nix in the style of nixpkgs, and
                      a flake that builds them
    aur/              the split PKGBUILD of smspp-project and the one of
                      stopt, with their .SRCINFO
    msys2/            the PKGBUILD of mingw-w64-smspp
    xmake/            an xmake repository with smspp, netcdf-cxx4 and stopt

## How a module is packaged in each channel

The split is the one of the PPA where the channel splits packages, and
elsewhere the one of vcpkg, i.e., one package with a switch per module, which
brings in the modules it needs:

| Channel | Package | One module | Module switch |
|---|---|---|---|
| Conan | `smspp/<v>` | `-o "smspp/*:sddp=False"` | option `<acr>` |
| Spack | `smspp` | `spack install smspp ~sddp` | variant `<acr>` |
| Nix | `smspp` | `smspp.override { modules = [ "ucblock" ]; }` | `modules` |
| AUR | `libsmspp-<acr>`, `smspp-<tool>`, `smspp-project` | `libsmspp-ucblock` | package |
| MSYS2 | `mingw-w64-<arch>-smspp` | none | none |
| xmake | `smspp` | `{configs = {sddp = false}}` | config `<acr>` |

The AUR package is split as the PPA is (one library package per module, one
command package per tool and the metapackage `smspp-project`), except that,
as usual on Arch, the headers and the CMake configuration of a module are in
its library package. MSYS2 has one package, as most of its C++ libraries do.
The libraries are shared in all the channels: the classes of a module
register themselves in the factories from static objects, which a static
library loses unless the program takes it with `--whole-archive`. A consumer
finds a module by the CMake configuration that SMS++ installs, e.g.,
`find_package(UCBlock)` and the target `SMS++::UCBlock`, which also keeps the
library at link time (in xmake, the package lists the libraries in a link
group without `--as-needed` instead).

The externals that a channel carries, and hence the modules and the
backends of its packages, differ somewhat from channel to channel:

| | Conan | Spack | Nix | AUR | MSYS2 | xmake |
|---|---|---|---|---|---|---|
| HiGHS | yes | ours | yes | yes | yes | yes |
| SCIP | option | no | yes | yes | no | no |
| qpOASES | yes | no | yes | no | no | yes |
| ProxQP, OSQP | no | no | yes | no | no | no |
| StOpt (SDDPBlock) | ours | ours | ours | ours | no | ours |
| LEMON (MCFLemonSolver) | yes | yes | yes | yes | no | yes |
| CaDiCaL, MiniSat (SATBlock) | no | no | yes | yes | no | no |
| libsvm, liblinear (SVMBlock) | libsvm | no | yes | yes | no | libsvm |

where "ours" is a recipe of this directory. A module whose required
dependency is missing is left out of the channel (SDDPBlock, InvestmentBlock
and MCFLemonSolver in MSYS2), while a missing optional one only takes away
the Solvers that wrap it (e.g., SATBlock and SVMBlock are built without their
solvers). OSQP is left out of the AUR package, since the static library of
the Arch package, which MILPSolver prefers, is made of slim LTO objects that
a build without LTO cannot link. SCIP is an option, off by default, in Conan,
whose recipe of SCIP builds the suite from the sources (I have not built SMS++
with it). A recipe turns off the search of what it does not carry, and points
the CMake of the modules to the externals of its channel (the
`<package>_ROOT` variables): the umbrella reads defaults for them from
`extlib/`, several under `/opt`, which might otherwise be picked up on the
machine that builds the package. For the same reason it turns off the RECORD
and COMBO sources of BinaryKnapsackBlock (COMBO is for non-commercial use
only).

## A release

The recipes are written for the release in `release.json`. After the tag of
the umbrella has published the tarball, in a checkout of `develop`

    packaging/gen_recipes.py --release X.Y.Z
    git diff packaging/
    git commit -m "recipes of the release X.Y.Z" packaging/

downloads the tarball, writes its SHA-256 into `release.json` and rewrites
the recipes. Then the channels take the new recipes as follows (the steps
need an account on the channel, and none of them is automated here).

### Conan

Conan Center carries neither netCDF-C++, which the core needs, nor StOpt.
Its rules also want the libraries of a recipe described as components, while
this one uses the CMake configuration that SMS++ installs. Hence, the recipes
are meant for a remote of the project, e.g., the Conan registry of the
GitLab project (the remote below), out of which the users install SMS++ with
the dependencies of Conan Center:

    conan remote add smspp \
        https://gitlab.com/api/v4/projects/smspp%2Fsmspp-project/packages/conan
    conan remote login smspp <user> -p <token with api scope>
    conan create packaging/conan/netcdf-cxx4 --version 4.3.1 --build=missing
    conan create packaging/conan/stopt --version 7.1 --build=missing
    conan create packaging/conan/smspp --version X.Y.Z --build=missing \
        -s compiler.cppstd=20
    conan upload "netcdf-cxx4/*" "stopt/*" "smspp/*" -r smspp -c

The first two are uploaded once, and again only when their versions
change; a user then writes

    conan remote add smspp <the URL above>
    conan install --requires=smspp/X.Y.Z --build=missing -s compiler.cppstd=20

and `find_package(UCBlock)` in the consumer, whose `CMAKE_PREFIX_PATH` is
set by the toolchain of Conan (cf. `conan/smspp/test_package`).

### Spack

The repository can be used as it is from a checkout of the umbrella,

    spack repo add packaging/spack/spack_repo/smspp
    spack install smspp
    spack test run smspp

For the builtin repository of Spack, the three packages go to a fork of
[spack/spack-packages](https://github.com/spack/spack-packages) under
`repos/spack_repo/builtin/packages/` (as `smspp`, `stopt` and `highs`), with
the maintainer added by `maintainers("<github user>")`; then `spack style`
and `spack audit packages smspp stopt highs` check them, and one pull request
proposes them.

### Nix

The flake builds the release with nothing else installed,

    nix build "gitlab:smspp/smspp-project?dir=packaging/nix"
    nix flake check "gitlab:smspp/smspp-project?dir=packaging/nix"

and `nix flake update` in `packaging/nix` moves `flake.lock` to a newer
nixpkgs. For nixpkgs, `stopt.nix` and `package.nix` go to a fork of
[NixOS/nixpkgs](https://github.com/NixOS/nixpkgs) as
`pkgs/by-name/st/stopt/package.nix` and `pkgs/by-name/sm/smspp/package.nix`,
with the maintainer added to `maintainers/maintainer-list.nix` and to their
`meta.maintainers`; they are formatted by `nixfmt` and built by `nix-build -A
smspp` before the pull request.

### AUR

The two packages are git repositories of the AUR, pushed from an account
with an SSH key, `stopt` first, since `smspp-project` builds on it:

    git clone ssh://aur@aur.archlinux.org/stopt.git
    cp packaging/aur/stopt/{PKGBUILD,.SRCINFO} stopt/
    (cd stopt && git add PKGBUILD .SRCINFO && git commit -m "X.Y" && git push)

and the same with `smspp-project`. The `.SRCINFO` is written by the
generator, the same file that `makepkg --printsrcinfo` writes on Arch.
`liblinear` is in the AUR too, and an AUR helper usually builds it first.

### MSYS2

The PKGBUILD goes to a fork of
[msys2/MINGW-packages](https://github.com/msys2/MINGW-packages) as
`mingw-w64-smspp/PKGBUILD`, in a pull request, whose CI builds it for the
environments of `mingw_arch`. Three things have to hold in the sources first,
since a build of the umbrella with MinGW stops on them: (i) the core uses
`std::atomic_flag` (`Observer.h`, `Solver.h`) without including `<atomic>`,
which the libc++ of the `clang64` and `clangarm64` environments does not
include on its behalf; (ii) the Windows layer of FastFlow, which
ThermalUnitDPSolver of UCBlock and SVMBlock include, is written for MSVC
(`ff/platforms/liblfds.h` has no branch for MinGW, and `platform.h` redefines
what MinGW already has); (iii) a module asks the linker to keep its library
only under MSVC (`/INCLUDE:SMSpp_force_load_<module>`) and on ELF and macOS,
and the import of a DLL that a tool does not call is dropped, so that the
factories are empty, while `LINKER:-u,SMSpp_force_load_<module>` under
`MINGW` keeps it. The name `netcdf_c++4` of the library of netCDF-C++ in
MSYS2, which the Find module of the core looks for only on Unix, is given by
the recipe itself.

### xmake

From a checkout of the umbrella the repository can be used as it is,

    xrepo add-repo smspp <checkout>/packaging/xmake
    xrepo install smspp

or with `add_repositories("smspp <checkout>/packaging/xmake")` in the
`xmake.lua` of a project. For
[xmake-repo](https://github.com/xmake-io/xmake-repo) the three directories of
`xmake/packages` go to a fork, on the branch `dev`, in a pull request whose CI
builds them on its platforms; `netcdf-cxx4` and `stopt` can also be proposed
first, by themselves.

## A new module

`init.sh --umbrella` of the ModuleTemplate adds the row of a new module to
`MODULES` in `modules.py` and runs `gen_recipes.py`; a new tool is one row of
`TOOLS`, written by hand. The external dependencies of the module are also
left to the hand. An external is named in `modules.py` by an abstract name
(e.g., `highs` or `stopt`), which goes into the required or the optional
list of the row; a new one is mapped to its package in each channel, i.e.,
in `ARCH` and `MSYS2` of `gen_recipes.py` and in the table of the externals
of `conan/smspp/conanfile.py`, `spack/.../smspp/package.py`,
`nix/package.nix` and `xmake/packages/s/smspp/xmake.lua` (and in
`CMAKE_NAME` and `ROOT_VAR` of `modules.py` when a Find module of SMS++ looks
for it).

## How the recipes were tried

I have built the recipes, the MSYS2 one excepted, on Linux x86_64, out of a
tarball made from `develop` as the release job makes it (hence the version
0.7.99 in `release.json`), with `gen_recipes.py --url-base` pointing to a
local copy of it. Conan was tried by `conan create` and its test package,
Spack by `spack install` and `spack test run`, Nix by `nix build` of the
package and of the check of the flake, and xmake by `xmake require` and a
consumer project. The AUR packages were built by `makepkg` in an Arch
container, then installed, and their tools solved the examples installed
with them. A consumer that finds UCBlock and MILPSolver by CMake and makes a
`UCBlock` and a `HiGHSMILPSolver` by the factories (`nix/consumer`) was built
against each channel. Of the MSYS2 recipe, `makepkg --printsrcinfo` reads the
PKGBUILD, and a cross build on Linux with its options (llvm-mingw, the
`clang64` packages of MSYS2 as the sysroot, and wine to run the generators
and the tools) gave the three defects above; with them worked around, the
core and 21 of the 23 modules of the recipe built (all but UCBlock and
SVMBlock), and the min-cost flow tool solved its example under wine.
