# 2. Installation and first build {#ch-2}

This chapter is deliberately short. The
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
and the umbrella project's `README.md` are the authoritative,
always-current source for the detailed installation steps; what
follows is an orientation, plus the one idea — the two coexisting
build systems — that a newcomer most needs to understand before
consulting them.

## 2.1 Dependencies {#sec-2-1}

The core SMS++ library requires a C++-20 compiler (a recent
`g++`, `clang++`, or MSVC), [Boost](https://www.boost.org), the
C++ interface to [netCDF](https://www.unidata.ucar.edu/software/netcdf/)
(`netCDF-cxx4`), and [Eigen](https://eigen.tuxfamily.org).

Beyond the core, individual modules pull in optional dependencies,
each of which can be omitted if the corresponding functionality is
not needed:

- the `MILPSolver` family wraps the MIP back-ends CPLEX, Gurobi,
  [SCIP](https://scipopt.org/) and [HiGHS](https://highs.dev) (the
  last two open-source);
- `MCFSolver` wraps the legacy
  [MCFClass](https://github.com/frangio68/Min-Cost-Flow-Class)
  algorithms, `MCFLemonSolver` wraps [LEMON](https://lemon.cs.elte.hu);
- `SDDPSolver` wraps the SDDP engine of the
  [StOpt](https://gitlab.com/stochastic-control/StOpt) project.

For a first installation, the project provides "zero-waste"
scripts that fetch and install the dependencies in default
locations: `INSTALL.sh` (Linux / macOS) and `INSTALL.ps1`
(Windows), each accepting `--without-<dependency>` flags
(`--without-cplex`, `--without-gurobi`, `--without-scip`,
`--without-highs`, `--without-stopt`, `--without-lemon`,
`--without-coinor`, `--without-smspp`) to skip the parts not wanted. The exact
invocations are in the project `README.md`; the Wiki's
[requirements guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#requirements)
has the detail.

## 2.2 The umbrella project and its modules {#sec-2-2}

SMS++ is distributed as an *umbrella* project,
[`smspp-project`](https://gitlab.com/smspp/smspp-project), whose
role is to provide a one-stop way to download and build all the
related modules together (and to produce a unified Doxygen and
track cross-module issues). At any time it points to the latest
releases of: the core library
([`smspp`](https://gitlab.com/smspp/smspp)), the leaf and
composite `:Block`s (`MCFBlock`, `BinaryKnapsackBlock`,
`CapacitatedFacilityLocationBlock`, `MMCFBlock`, `UCBlock`,
`SDDPBlock`, `StochasticBlock`, `TwoStageStochasticBlock`,
`InvestmentBlock`, ...), the `:Solver`s (`MILPSolver`,
`LagrangianDualSolver`, `BundleSolver`, ...), and the shared
`tests` and `tools` repositories. Cloning the umbrella (with its
sub-modules) is the recommended way to obtain a coherent set.

## 2.3 Two build systems, two use cases {#sec-2-3}

SMS++ can be built either with [CMake](https://cmake.org) or with
plain makefiles, and the choice is not arbitrary — the two suit
different workflows.

**CMake — for "fire and forget".** CMake builds *off-source* (the
artefacts go into a separate build tree). This makes it the better
choice for a one-off "build, install, and forget" installation:
the developer who simply wants SMS++ available to link against
from their own project configures once, builds, installs, and
never thinks about it again. The standard interactive front-end
[`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
makes the configuration step pleasant: it presents the available
options — which optional solver interfaces to enable, where
dependencies live, debug vs release — as a menu to toggle, without
hand-editing any file. Several configuration options are
documented in the Wiki's
[customisation page](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration).

**Makefiles — for the developer loop.** Every module, starting
with the core, also ships hand-crafted makefiles that build
*on-source* (the artefacts sit next to the source). The on-source
build is much better suited to the tight "edit, compile, test"
loop of someone *developing* SMS++ components, because the build
products are right there and the loop is short. The core library
is built by `SMS++/lib/makefile-lib` into `lib/libSMS++.a`; an
executable's own makefile then declares which modules it needs.

The module-dependency mechanism in the makefiles rests on two
include files that *every* module provides:

- `makefile-c` — the "complete" include: it pulls in the module's
  own headers and compilation rules, *all the modules it depends
  on* (which by definition includes the core), and all the
  external libraries.
- `makefile-s` — the "sub" include: the same, but *without* the
  core, so that the core is not pulled in more than once.

The rule an executable's makefile follows is therefore simple:
**include exactly one `makefile-c`** — the one for the module that
brings in the core — **and include every other module through its
`makefile-s`.** This keeps the core's headers and objects in the
build exactly once while letting an executable compose any set of
modules. The existing testers (`MCFBlock/test`,
`tests/CapacitatedFacilityLocation`, ...) are the reference
examples to copy from.

In both build systems, external dependencies are found
automatically if they sit in their OS-default locations; if not,
the supported way to point at them is to copy the right
`extlib/makefile-default-paths-<os>` file to `extlib/makefile-paths`
(which is `.gitignore`-d, so local settings never leak into the
repository) and set the `*_ROOT` values there. This single file is
read by both CMake and the makefiles.

## 2.4 First smoke tests {#sec-2-4}

A point worth understanding here is *why the tests are organised
the way they are*. A `:Block` on its own is, in general, not very
testable: without an appropriate `:Solver` one cannot even
`compute()` it, and it would be wrong to make a `:Solver` a
*prerequisite* of a `:Block` (the whole point of the framework is
that `:Block`s and `:Solver`s are independent). This is precisely
why `tests/` is a **separate repository**: the testers that need
both a `:Block` and a `:Solver` from different modules live there,
not inside any one module. Among the running examples,
`tests/CapacitatedFacilityLocation` is the one used by Recipes [R3](R3-cfl-three-ways.md#rec-R3)–[R5](R5-cfl-benders.md#rec-R5).

Some `:Block`s, however, *do* carry their own tester under a local
`test/` directory — namely those that "come with a `:Solver`
attached" and so are self-contained: `MCFBlock` (with
`MCFSolver`), `BinaryKnapsackBlock` (with
`DPBinaryKnapsackSolver`), `CapacitatedFacilityLocationBlock`, and
`SDDPBlock`. Building and running one of these is the quickest way
to confirm that the toolchain, the dependencies and the paths are
all in order. The `MCFBlock/test` tester — the runnable
counterpart of [Recipe R1](R1-mcf.md#rec-R1) — is a good first target: it builds the
core, the `MCFBlock` module and the `MCFSolver`, loads a small flow
instance, solves it, and checks the result.

## 2.5 Troubleshooting {#sec-2-5}

Two failure modes are common enough to flag here, both covered in
depth elsewhere in this manual or in the Wiki:

- a runtime error of the form *"XXXX not present in YYYY
  factory"*, despite the class being in the source tree, is almost
  always the linker having optimised away a translation unit whose
  only job was to register a class; [§18.2](18-factories-netcdf.md#sec-18-2) explains the cause and
  the fixes (linker flags or `SMSpp_ensure_load`).
- a dependency that the build cannot find is almost always a
  `*_ROOT` path issue; copy `extlib/makefile-default-paths-<os>`
  to `extlib/makefile-paths` and correct the relevant value, as in
  [§2.3](02-installation.md#sec-2-3).

For anything beyond these, the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
is the reference; it carries the full, platform-specific
installation and troubleshooting guides that this chapter
deliberately does not duplicate.
