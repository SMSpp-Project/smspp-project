# 2. Installation and first build {#ch-2}

We keep this chapter deliberately short. The
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
and the `README.md` of the umbrella project are the authoritative
(and always current) source of the detailed installation steps; what
follows is an orientation, plus the one idea that a newcomer most
needs before consulting them, i.e., that two build systems coexist,
each made for a different use.

## 2.1 Dependencies {#sec-2-1}

Its core library requires a C++20 compiler (a recent `g++`,
`clang++` or MSVC), [Boost](https://www.boost.org), the C++ interface
to [netCDF](https://www.unidata.ucar.edu/software/netcdf/)
(`netCDF-cxx4`) and [Eigen](https://eigen.tuxfamily.org); the CMake
build also fetches the header-only
[FastFlow](https://github.com/fastflow/fastflow) library at
configuration time, and therefore needs the network unless a local
copy is given.

Beyond the core, the individual modules require optional
dependencies, each of which can be omitted if the corresponding
functionality is not needed:

- the `MILPSolver` family wraps the MIP solvers CPLEX, Gurobi,
  MOSEK, [SCIP](https://scipopt.org/) and [HiGHS](https://highs.dev)
  (the last two open source), the interior-point solver PIPS-IPM++
  for LPs, and the open-source convex QP solvers DAQP, qpOASES,
  ProxQP, PIQP, OSQP, qpmad, HPIPM, quadprog, Clarabel and PermonQP;
- `MCFSolver` wraps the algorithms of the
  [MCFClass](https://github.com/frangio68/Min-Cost-Flow-Class)
  library, and `MCFLemonSolver` those of
  [abLEMON](https://github.com/dmeoli/abLEMON), the maintained fork
  of [LEMON](https://lemon.cs.elte.hu);
- `SDDPSolver` wraps the SDDP engine of the
  [StOpt](https://gitlab.com/stochastic-control/StOpt) project;
- the `:Solver`s of `SATBlock` wrap CaDiCaL and MiniSat, those of
  `SVMBlock` LIBSVM and LIBLINEAR, two of those of
  `BinaryKnapsackBlock` the RECORD and COMBO codes, and
  `BundleSolverML` uses Torch.

For a first installation, the project provides scripts that fetch and
install the dependencies in default locations. The first is
`INSTALL.sh` (Linux and macOS), which accepts `--without-<dependency>`
flags (`--without-cplex`, `--without-gurobi`, `--without-scip`,
`--without-highs`, `--without-stopt`, `--without-lemon`,
`--without-smspp`, and so on); the second is `INSTALL.ps1` (Windows),
which accepts the corresponding `-without<Dependency>` switches
(`-withoutCplex`, `-withoutGurobi`, `-withoutSCIP`, `-withoutSMSpp`,
and a few others: there, most of the open-source dependencies come
from vcpkg and have no switch). Run again, the scripts update what
they have installed. Both the exact invocations and the full table of
the options are in the `README.md` of the project, and the
[requirements
guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#requirements)
of the Wiki has the details.

SMS++ is also distributed in binary form: as Ubuntu packages from the
PPA `ppa:smspp-project/smspp` (one package per module and per tool,
plus the metapackage `smspp-project`), on conda-forge
(`smspp-project`), in the Homebrew tap `SMSpp-Project/smspp`, and as a
vcpkg port (with one feature per module) in the [SMS++ vcpkg
registry](https://gitlab.com/smspp/vcpkg-registry). The recipes for
Conan, Spack, Nix, the AUR, MSYS2 and xmake are in the `packaging/`
directory of the umbrella. All these packages only carry some of the
back-ends of `MILPSolver` (HiGHS, DAQP and qpOASES in all of them),
since CPLEX, Gurobi and MOSEK cannot be redistributed; a build against
them, or against SCIP, is a build from the sources.

## 2.2 The umbrella project and its modules {#sec-2-2}

SMS++ is distributed as an *umbrella* project,
[`smspp-project`](https://gitlab.com/smspp/smspp-project), whose role
is to provide a single way to download and build all the related
modules together (as well as to produce a unified Doxygen reference
and to track the issues that span several modules). At any time it
points to the latest releases of the core library
([`smspp`](https://gitlab.com/smspp/smspp)), of the leaf and composite
`:Block`s (`MCFBlock`, `BinaryKnapsackBlock`,
`CapacitatedFacilityLocationBlock`, `MMCFBlock`, `UCBlock`,
`SDDPBlock`, `StochasticBlock`, `TwoStageStochasticBlock`,
`InvestmentBlock`, and so on), of the `:Solver`s (`MILPSolver`,
`LagrangianDualSolver`, `BundleSolver`, and so on), and of the shared
`tests` and `tools` repositories. It also carries, as submodules that
are initialized only on request, the Python packages `pySMSpp` and
`pypsa2smspp`, which CMake does not build and which are installed with
`pip`. The former is the Python interface of SMS++, which builds the
netCDF files of SMS++ models, runs the SMS++ tools on them and reads
their results back; the latter translates a PyPSA energy-system
network into the corresponding SMS++ model. Cloning the umbrella with
its submodules is the recommended way to obtain a consistent set of
modules.

## 2.3 Two build systems, two uses {#sec-2-3}

One can build SMS++ either with [CMake](https://cmake.org) or with
plain makefiles, and the choice is not entirely arbitrary, since the
two suit rather different workflows.

CMake builds *out of source*, i.e., the products of the build go to a
separate tree. This makes it the better choice for a one-off
installation: one who simply wants SMS++ available to link against
from their project configures once, builds, installs, and need not
think about it again. Its standard interactive front-end
[`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
presents the options of the configuration (which optional interfaces
to external solvers to enable, where the dependencies are, a debug or
a release build) as a menu, without editing any file. Each module is
switched on and off by its `BUILD_<module>` option (e.g.,
`BUILD_UCBlock` or `BUILD_MILPSolver`), which is on by default for the
submodules that are checked out, and which also switches on the
modules it requires. Several options of the configuration are
described in the [customization
page](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration)
of the Wiki.

The makefiles, instead, build *in source*, i.e., the products of the
build sit next to the sources; each module, starting from the core,
ships its makefiles. This is better suited to the short "edit,
compile, test" loop of one who develops SMS++ components, since the
products are where the sources are. In particular, the core library is
built by `SMS++/lib/makefile-lib` into `SMS++/lib/libSMS++.a`, and the
makefile of an executable declares which modules it needs.

The dependencies between modules rest on two include files that the
modules other than the core provide in their root directory. The
"complete" one, `makefile-c`, brings in the headers and the
compilation rules of the module, those of all the modules it depends
on (comprising the core, whose `makefile-c` is in `SMS++/lib/`), and
all the external libraries. The "sub" one, `makefile-s`, does the same
but without the core, which therefore is not brought in more than
once. The rule for the makefile of an executable is therefore to
include one `makefile-c`, that of the module through which the core
comes in, and the other modules through their `makefile-s`. This keeps
the headers and the objects of the core in the build once, while
letting an executable compose any set of modules. Existing testers
(e.g., `MCFClassSolver/test` and
`tests/CapacitatedFacilityLocationBlock`) are the examples to copy
from.

In both build systems, the external dependencies are found
automatically if they are installed in the default locations of the
operating system. Otherwise, the supported way to point to them is to
copy the file `extlib/makefile-default-paths-<os>` to
`extlib/makefile-paths` (which git ignores, and therefore local
settings do not end up in the repository) and to set there the
`*_ROOT` values; the same file is read by both CMake and the
makefiles.

## 2.4 First tests {#sec-2-4}

How the tests are organized follows from one design choice. A `:Block`
by itself is, in general, hardly testable, since without a `:Solver`
one cannot even `compute()` anything on it; yet a `:Solver` must not
be a prerequisite of a `:Block`, because `:Block`s and `:Solver`s are
meant to be independent. This is why `tests/` is a separate
repository: the testers that need a `:Block` and a `:Solver` of
different modules live there instead of in either of the two modules.
Among the running examples, `tests/CapacitatedFacilityLocationBlock`
is the one used by recipes [R3](R3-cfl-three-ways.md#rec-R3) to
[R5](R5-cfl-benders.md#rec-R5).

Most modules, however, also carry a tester in their `test/` directory,
and in particular those that come with a `:Solver` and are therefore
self-contained: `MCFClassSolver` (whose `MCFSolver` solves an
`MCFBlock`), `BinaryKnapsackBlock` (with `DPBinaryKnapsackSolver`) and
`CapacitatedFacilityLocationBlock`. Building and running one of these
is the quickest way to check that the toolchain, the dependencies and
the paths are in order. A good first target is the tester in
`MCFClassSolver/test`, which is the runnable counterpart of [Recipe
R1](R1-mcf.md#rec-R1): it builds the core, the `MCFBlock` module and
`MCFSolver`, loads small flow instances, solves them and checks the
results.

## 2.5 Troubleshooting {#sec-2-5}

Two failures are common enough to be mentioned here, both covered in
detail elsewhere in this manual or in the Wiki. First, an error at run
time of the form "XXXX not present in YYYY factory", although the
class is in the source tree, is almost always due to the linker having
discarded a translation unit whose only purpose was to register a
class in a factory. [§18.2](18-factories-netcdf.md#sec-18-2) explains
the cause and the remedies (linking the module through its CMake
target, or a flag of the linker that keeps the unreferenced units).
Second, a dependency that the build cannot find is almost always a
wrong `*_ROOT` path, to be corrected in `extlib/makefile-paths` as
described in [§2.3](02-installation.md#sec-2-3).

For anything else, the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
is the reference: it has the complete installation and
troubleshooting guides of each platform, which this chapter
deliberately does not duplicate.
