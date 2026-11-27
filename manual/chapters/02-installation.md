# 2. Installation and first build {#ch-2}

[Source: [`README.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/README.md), [`CMakeLists.txt`](https://gitlab.com/smspp/smspp-project/-/blob/develop/CMakeLists.txt), [`CMakeSettings.txt`](https://gitlab.com/smspp/smspp-project/-/blob/develop/CMakeSettings.txt) and [`INSTALL.sh`](https://gitlab.com/smspp/smspp-project/-/blob/develop/INSTALL.sh) of the umbrella;
[`SMS++/cmake/SMSppModule.cmake`](https://gitlab.com/smspp/smspp/-/blob/develop/cmake/SMSppModule.cmake), [`CheckModule.cmake`](https://gitlab.com/smspp/smspp/-/blob/develop/cmake/CheckModule.cmake),
[`SMS++/lib/makefile-c`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-c), [`makefile-shared`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-shared), [`makefile-mod`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-mod)]

We are concerned in this chapter with how one obtains SMS++ and builds
it for the first time, and we keep it deliberately short, since the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home) is
the authoritative (and always current) source of the detailed
installation steps. What follows is an
orientation, plus the one idea that a newcomer most needs before
consulting them, i.e., that two build systems coexist, each made for a
different use. We list the dependencies and the binary distributions
([§2.1](02-installation.md#sec-2-1)), and describe the umbrella project
([§2.2](02-installation.md#sec-2-2)) and the two build systems
([§2.3](02-installation.md#sec-2-3)). We then point to the testers
that are the natural first targets
([§2.4](02-installation.md#sec-2-4)), and close with the two failures
that are common enough to be discussed here
([§2.5](02-installation.md#sec-2-5)).

## 2.1 Dependencies {#sec-2-1}

The core library of SMS++ requires a C++20 compiler (a recent `g++`,
`clang++` or MSVC), [Boost](https://www.boost.org) (version 1.72 or
later, headers only), the C++ interface
to [netCDF](https://www.unidata.ucar.edu/software/netcdf/)
(`netCDF-cxx4`) and [Eigen](https://eigen.tuxfamily.org); the CMake
build also fetches the header-only
[FastFlow](https://github.com/fastflow/fastflow) library at
configuration time, and therefore needs the network, unless a local
copy is given by `FETCHCONTENT_SOURCE_DIR_FASTFLOW`. A release
tarball, instead, carries FastFlow (and the RECORD code of
`BinaryKnapsackBlock`) in its `third-party/` directory.

Beyond the core, the individual modules require optional
dependencies, each of which can be omitted if the corresponding
functionality is not needed:

- the `MILPSolver` family wraps the MIP solvers CPLEX, Gurobi,
  MOSEK, [SCIP](https://scipopt.org/) and [HiGHS](https://highs.dev)
  (the last two open source), the interior-point solver PIPS-IPM++
  for LPs and for convex QPs whose Hessian is separable by sub-`Block`,
  and the open-source convex QP solvers DAQP, qpOASES, ProxQP, PIQP,
  OSQP, qpmad, HPIPM, quadprog, Clarabel and PermonQP (each back-end is
  built if the CMake option `MILPSolver_USE_<X>`, e.g.,
  `MILPSolver_USE_OSQP`, is on, as it is by default, and its library is
  found);
- `MCFSolver` wraps the algorithms of the
  [MCFClass](https://github.com/frangio68/Min-Cost-Flow-Class)
  library, and `MCFLemonSolver` those of
  [abLEMON](https://github.com/dmeoli/abLEMON), the maintained fork
  of [LEMON](https://lemon.cs.elte.hu);
- `SDDPSolver` wraps the SDDP engine of the
  [StOpt](https://gitlab.com/stochastic-control/StOpt) project;
- the `:Solver`s of `SATBlock` wrap CaDiCaL and MiniSat, those of
  `SVMBlock` LIBSVM and LIBLINEAR, and two of those of
  `BinaryKnapsackBlock` the RECORD and COMBO codes;
- `BundleSolverML`, `LagrangianDualRelaxationSolverML` and the
  branching rule `GQSATBranchRule` of `SATBlock` use Torch; each of them
  is in a library of its own (`SMS++::BundleSolverML`,
  `SMS++::LagrangianDualSolverML` and `SMS++::SATBlockML`, respectively),
  built only if Torch is found, and hence a program that does not use
  them does not load Torch;
- `FlatZincCSPSolver` of `CSPBlock` (since SMS++ 0.9.0) runs an external
  FlatZinc solver as a separate process, typically the CP-SAT, Gecode or
  Chuffed of the [MiniZinc](https://www.minizinc.org) bundle, which is
  needed only at run time, and which the installation scripts below
  install unless they are given `--without-flatzinc`
  (`-withoutFlatzinc`).

For a first installation, the project provides scripts that fetch and
install the dependencies in default locations. The first is
[`INSTALL.sh`](https://gitlab.com/smspp/smspp-project/-/blob/develop/INSTALL.sh)
(Linux and macOS), which accepts `--without-<dependency>` flags
(`--without-cplex`, `--without-gurobi`, `--without-mosek`,
`--without-scip`, `--without-highs`, `--without-stopt`,
`--without-torch`, `--without-lemon`, `--without-flatzinc`,
`--without-smspp`, and so on); the second is `INSTALL.ps1` (Windows),
which accepts the corresponding `-without<Dependency>` switches
(`-withoutCplex`, `-withoutGurobi`, `-withoutSCIP`, `-withoutSMSpp`,
and a few others: there, most of the open-source dependencies come
from vcpkg and have no switch). Run again, the scripts update what
they have installed. Both the exact invocations and the full table of
the options are in the [installation
guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#one-shot-install-scripts)
of the Wiki, whose [requirements
section](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#requirements)
has the details of each dependency.

SMS++ is also distributed in binary form: as Ubuntu packages from the
PPA `ppa:smspp-project/smspp` (one package per module and per tool,
plus the metapackage `smspp-project`), on conda-forge
(`smspp-project`), in the Homebrew tap `SMSpp-Project/smspp`, and as a
vcpkg port (with one feature per module) in the [SMS++ vcpkg
registry](https://gitlab.com/smspp/vcpkg-registry). The recipes for
Conan, Spack, Nix, the AUR, MSYS2 and xmake are in the `packaging/`
directory of the umbrella. The PPA, conda-forge, Homebrew and vcpkg
packages carry the HiGHS, DAQP and qpOASES back-ends of `MILPSolver`
(vcpkg also ProxQP, PIQP, OSQP and qpmad), and the other channels those
that they have (HiGHS always, SCIP in Nix, in the AUR and as an
option in Conan; see the
[`packaging/README.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/packaging/README.md)).
No package carries CPLEX, Gurobi or MOSEK, which cannot be
redistributed. The recipes of Conan, Spack and Nix can, however, build
the back-ends against a local installation of each, while any other
build against them is a build from the sources.

## 2.2 The umbrella project and its modules {#sec-2-2}

SMS++ is distributed as an *umbrella* project,
[`smspp-project`](https://gitlab.com/smspp/smspp-project), whose role is to
provide a single way to download and build all the related modules together
(as well as to produce a unified Doxygen reference and to track the issues
that span several modules). At any time it points to the latest releases of
the core library ([`smspp`](https://gitlab.com/smspp/smspp)), of the leaf and
composite `:Block`s (`MCFBlock`, `BinaryKnapsackBlock`,
`CapacitatedFacilityLocationBlock`, `MMCFBlock`, `UCBlock`, `SDDPBlock`,
`StochasticBlock`, `TwoStageStochasticBlock`, `InvestmentBlock`, and so on),
of the `:Solver`s (`MILPSolver`, `LagrangianDualSolver`, `BundleSolver`, and
so on), and of the shared `tests` and `tools` repositories. It also carries,
as submodules, the Python packages `pySMSpp` and `pypsa2smspp`, which CMake
neither initializes nor builds, and which are installed with `pip`. The former
is the Python interface of SMS++, which builds the netCDF files of SMS++
models, runs the SMS++ tools on them and reads their results back; the latter
translates a PyPSA energy-system network into the corresponding SMS++ model.
Cloning the umbrella with its submodules is the recommended way to obtain a
consistent set of modules.

## 2.3 Two build systems, two uses {#sec-2-3}

One can build SMS++ either with [CMake](https://cmake.org) or with
plain makefiles, and the choice is not entirely arbitrary, since the
two suit rather different workflows.

CMake (version 3.24 or later, since SMS++ 0.9.0; 3.21 for version
0.8.0) builds *out of source*, i.e., the
products of the build go to a separate tree. This makes it the better
choice for a one-off installation: one who simply wants SMS++
available to link against from their project configures once, builds,
installs, and need not think about it again. A CMake project then
finds it by `find_package(SMS++)`, and links the target
`SMS++::SMS++` (or the target of any other module). Its standard
interactive front-end
[`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
presents the options of the configuration (which optional interfaces
to external solvers to enable, where the dependencies are, a debug or
a release build) as a menu, without editing any file. Each module is
switched on and off by its `BUILD_<module>` option (e.g.,
`BUILD_UCBlock` or `BUILD_MILPSolver`), which is on by default for the
submodules that are checked out, and which also switches on the
modules it requires; a module that is not checked out is fetched when
its option is turned on. The build type is `Release` unless one is
given. Several options of the configuration are
described in the [customization
page](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-Configuration)
of the Wiki, and we describe two of them here.

- The umbrella builds each module as a shared library, as set in its
  `CMakeSettings.txt`; a build with static libraries, obtained by changing
  that setting, needs (since SMS++ 0.9.0) CMake 3.30, which links each archive
  whole. With shared libraries, the library of each module is also a *plugin*
  (since SMS++ 0.9.0): a program linked to the core alone can construct,
  through the factories, any `:Block` and any `:Solver` of the modules that
  are built or installed, since the library that registers the class is loaded
  when the class is first asked for. The plugins are the subject of
  [§18.8](18-factories-netcdf.md#sec-18-8), while
  [§18.7](18-factories-netcdf.md#sec-18-7) explains why all the shared
  libraries of a program reach the same factories.
- With CMake 3.28 or later, the option `SMSpp_BUILD_MODULE` builds the C++20
  module of the core, `smspp`, and that of each other module
  (`smspp.mcfblock`, `smspp.ucblock` and so on, as the target
  `SMS++::<module>_module`), so that a source file can write `import smspp;`
  in place of the `#include` of the headers; since a module exports no macro,
  a file that registers a class in a factory still includes `SMSFactory.h`.
  Unless it is given, the option is on wherever the generator and the compiler
  build a small module and a unit that imports it, as tried at configuration
  time, and off elsewhere. This is typically the case of Ninja with Clang 16
  or later and of MSVC 19.34 or later (on by default with MSVC since SMS++
  0.9.0, only with Clang in version 0.8.0), but not of GCC, up to version 15
  at least. Where the modules cannot be built, the build goes on with the
  headers.

The makefiles, instead, build *in source*, i.e., the products of the
build sit next to the sources; each module, starting from the core,
ships its makefiles. This is better suited to the short "edit,
compile, test" loop of one who develops SMS++ components, since the
products are where the sources are. As with CMake, the objects of each
module go by default into a shared library (a DLL on Windows), against
which an executable is linked
([`SMS++/lib/makefile-shared`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-shared),
since SMS++ 0.9.0; in version 0.8.0 the makefiles linked the objects
directly);
the executable finds it at run time by its run-time path, or on
Windows on the `PATH`. With `SMSppSTATIC` set (e.g.,
`make SMSppSTATIC=1`) the objects are linked as they are; the core
library can also be archived by
[`SMS++/lib/makefile-lib`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-lib),
run as `make -f makefile-lib` in `SMS++/lib/`, into `SMS++/lib/libSMS++.a`.
Also, with `SMSppMOD` set (e.g., `make SMSppMOD=1`), `clang++` 16 or
later builds the C++20 modules as well
([`SMS++/lib/makefile-mod`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-mod)),
while the plugins are a feature of the CMake build only.

The dependencies between modules rest on two include files that the
modules other than the core provide in their root directory. The
"complete" one, `makefile-c`, brings in the headers and the
compilation rules of the module, those of all the modules it depends
on (comprising the core, whose
[`makefile-c`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-c)
is in `SMS++/lib/`), and all the external libraries. The "sub" one,
`makefile-s`, does the same but without the core, which therefore is
not brought in more than once. The makefile of an executable
therefore includes one `makefile-c`, that of the module through which
the core comes in, and the other modules through their `makefile-s`,
as `MCFClassSolver/test` does. Alternatively, it brings in the core
through
[`SMS++/lib/makefile-inc`](https://gitlab.com/smspp/smspp/-/blob/develop/lib/makefile-inc),
which also keeps the library of the core up to date, and every other
module through its `makefile-s`, as
`tests/CapacitatedFacilityLocationBlock` does. Either way the headers
and the objects of the core are in the build once, while an executable
can compose any set of modules.

In both build systems, the external dependencies are found
automatically if they are installed in the default locations of the
operating system. Otherwise, one points to them by a
`-D<Lib>_ROOT=<path>` on the command line of CMake or, for both build
systems, by the `*_ROOT` values set in `extlib/makefile-paths`, which
both CMake and the makefiles read. The latter is a copy of
`extlib/makefile-default-paths-<os>`, which git ignores, so that local
settings do not end up in the repository.

## 2.4 First tests {#sec-2-4}

How the tests are organized follows from one design choice. A `:Block`
by itself is, in general, hardly testable, since without a `:Solver`
one cannot even `compute()` anything on it; yet a `:Solver` must not
be a prerequisite of a `:Block`, because `:Block`s and `:Solver`s are
meant to be independent. This is why `tests/` is a separate
repository: the testers that need a `:Block` and a `:Solver` of
different modules live there instead of in either of the two modules.
Among the testers of the running examples, that of
`tests/CapacitatedFacilityLocationBlock` is the one used by recipes
[R3](R3-cfl-three-ways.md#rec-R3) to [R5](R5-cfl-benders.md#rec-R5).

Most modules, however, also carry a tester in their `test/` directory,
and in particular those that come with a `:Solver` and are therefore
self-contained: `MCFClassSolver` (whose `MCFSolver` solves an
`MCFBlock`) and `BinaryKnapsackBlock` (with `DPBinaryKnapsackSolver`),
while the tester of `CapacitatedFacilityLocationBlock` tests its
feasibility checks with no `:Solver` attached. Building and running one
of these is the quickest way to check that the toolchain, the
dependencies and the paths are in order. A good first target is the
tester in `MCFClassSolver/test`, which is the runnable counterpart of
[Recipe R1](R1-mcf.md#rec-R1). It builds the core, the `MCFBlock`
module and `MCFSolver`, loads small flow instances and solves them
repeatedly while changing costs, capacities, deficits and arcs, and it
compares the results with those of the same `:MCFClass` used
directly.

## 2.5 Troubleshooting {#sec-2-5}

Two failures are common enough to be mentioned here, both covered in
detail elsewhere in this manual or in the Wiki. First, an error at run
time of the form "XXXX not present in YYYY factory", although the
class is in the source tree, means that no library of the program
has registered the class. With static libraries this is usually due to
the linker having discarded a translation unit whose only purpose was
to register a class in a factory, and
[§18.2](18-factories-netcdf.md#sec-18-2) explains the cause and the
remedies. With shared libraries built by CMake, instead, it
means that no plugin table names the class: either the module was not
built or installed, or its directory is neither one of those searched
by default nor in the environment variable `SMSpp_PLUGIN_PATH` (cf.
[§18.8](18-factories-netcdf.md#sec-18-8)). Second, a dependency that
the build cannot find is almost always a wrong `*_ROOT` path, to be
corrected as described in [§2.3](02-installation.md#sec-2-3).

For anything else, see the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home).
