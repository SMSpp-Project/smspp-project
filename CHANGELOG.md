# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `packaging/`, the recipes of SMS++ for Conan (with netcdf-cxx4 and stopt,
  for a remote of the project), Spack (a repository with smspp, stopt and
  highs), Nix (package.nix, stopt.nix and a flake), the AUR (the split
  package smspp-project, one package per module and per tool as in the PPA,
  and stopt), MSYS2 and xmake (a repository with smspp, netcdf-cxx4 and
  stopt), all written by `packaging/gen_recipes.py` from the table of the
  modules of `packaging/modules.py`, with the steps of a release for each
  channel in `packaging/README.md`

- The section "Build inside a CMake project" of the README, on building
  SMS++ along with a project by FetchContent or CPM.cmake

- `SMSPP_EXTLIB_DEFAULTS`, ON by default, which with OFF leaves out the
  default paths of `extlib/makefile-default-paths-*`, several under `/opt`,
  so that a package is not built against what its building machine has
  there; with the default nothing changes, `extlib/makefile-paths` is read
  either way

- A second site of the documentation, built by Sphinx with the theme of
  Read the Docs from `sphinx/` and served at `/smspp-project/sphinx/` next
  to the Doxygen site and the user manual: its pages come from the
  README.md, the general description of `doxygen/mainpage.md`,
  CONTRIBUTING.md and this CHANGELOG, and its API reference is built by
  Breathe and Exhale from the XML that `doxygen/Doxyfile` now writes into
  `doc/xml` along with the HTML, which is unchanged

- SubGradSolver, the deflected and projected subgradient methods for the
  same problems as BundleSolver (`BUILD_SubGradSolver`), and its feature
  `subgrad` in the smspp port of the vcpkg registry; the battery of
  MultiKnapsackAssignBlock cross-checks it as the inner `Solver` of
  `LagrangianDualSolver`

- `--with-experiments` in INSTALL.sh and `-withExperiments` in INSTALL.ps1,
  which fetch and build the private `experiments` submodule

- DAQP, the QP solver of `DAQPMILPSolver`: `INSTALL.sh` builds it from its
  latest tag (`--without-daqp`), INSTALL.ps1 takes it from the SMS++ vcpkg
  registry through `vcpkg.json`, and `extlib` has `makefile-libDAQP` and the
  default paths of `DAQP_ROOT`

- qpOASES, the QP solver of `QPOASESMILPSolver`: `INSTALL.sh` builds it
  from its latest tag as a shared library whose BLAS and LAPACK
  replacements are renamed (`--without-qpoases`), INSTALL.ps1 takes it from
  the default vcpkg registry through `vcpkg.json`, and `extlib` has
  `makefile-libqpOASES` and the default paths of `qpOASES_ROOT`

- `extlib` has `makefile-libClarabel` and the default paths of
  `Clarabel_ROOT` for `ClarabelMILPSolver`, whose C interface no package
  manager carries and `INSTALL.sh` does not install, as it needs Rust

- MOSEK 11, the commercial solver of `MOSEKMILPSolver`, which needs a
  license of its own (free for academic use): `INSTALL.sh` downloads the
  tools archive of the latest 11.0.x version on Linux and on Apple Silicon
  macOS (`--without-mosek`), `INSTALL.ps1` its installer on Windows
  (`-withoutMosek`), both say where the license goes, and `extlib` has
  `makefile-libMOSEK` and the default paths of `MOSEK_ROOT`; the CI image
  has it, its license coming from the file CI/CD variable
  `MOSEKLM_LICENSE_FILE` of each project

- ProxSuite, PIQP and OSQP, the QP solvers of `ProxQPMILPSolver`,
  `PIQPMILPSolver` and `OSQPMILPSolver`: `INSTALL.sh` builds them from
  their latest tag on Linux and macOS (`--without-proxsuite`,
  `--without-piqp`, `--without-osqp`), INSTALL.ps1 takes them from vcpkg
  through `vcpkg.json` (PIQP from the SMS++ vcpkg registry), and `extlib` has
  `makefile-libProxSuite`, `makefile-libPIQP`, `makefile-libOSQP` and the
  default paths of `proxsuite_ROOT`, `piqp_ROOT` and `osqp_ROOT`

- qpmad, the QP solver of `QPMADMILPSolver`: `INSTALL.sh` installs its
  headers from its latest tag on Linux and macOS (`--without-qpmad`),
  INSTALL.ps1 takes it from the SMS++ vcpkg registry through `vcpkg.json`,
  and `extlib` has `makefile-libqpmad` and the default paths of `qpmad_ROOT`

- quadprog, the QP solver of `QuadProgMILPSolver`: being under the GPL, it
  is installed only by `INSTALL.sh --with-quadprog`, on Linux and macOS,
  which compiles its C sources from its latest tag into `libquadprog`, which
  exports only `qpgen2_`; it is in no package, and `extlib` has
  `makefile-libquadprog` and the default paths of `quadprog_ROOT`

### Changed

- the release tarball carries FastFlow and RECORD, with their licenses, in
  `third-party/`, at the commits of `packaging/modules.py`
  (`packaging/third_party.py`), and the `CMakeLists.txt` of the umbrella
  uses them in place of the FastFlow that SMS++ fetches and of a RECORD
  checkout, so that a build of the tarball fetches nothing

- `INSTALL.sh` and `INSTALL.ps1` install the latest MOSEK 12.0.x in place
  of 11.0.x (12.0.0 if its number cannot be read), so does the image of
  the CI that is built by `INSTALL.sh`; MOSEKMILPSolver still builds with
  MOSEK 10 or later, and a license of MOSEK 12 serves the earlier versions

### Fixed

- INSTALL.sh builds ProxSuite 0.8.0, its latest release, which has no
  `cmake-module` submodule any more and needs cereal unless its
  serialization is turned off

- the tests of the modules are reachable by ctest from the build root in
  a fresh build too, where `BUILD_TESTING` was not yet defined when it was
  checked and the build root had no test

- the tests run from the build root have no timeout, as before the
  umbrella included CTest, whose default of 1500 s stopped the batteries
  of LagBFunction and PolyhedralFunctionBlock in the CI of BundleSolver

- INSTALL.sh and INSTALL.ps1 no longer touch the private `experiments`
  submodule unless asked to, whatever the local git configuration says, so
  that an installation never stops to ask for GitLab credentials

## [0.7.0] - 2026-10-09

### Added

- MultiKnapsackAssignBlock, the `Block` of the Multiple Knapsack Assignment
  Problem, whose sub-`Block` are `BinaryKnapsackBlock`
  (`BUILD_MultiKnapsackAssignBlock`, which turns on
  `BUILD_BinaryKnapsackBlock`), and its feature `mkab` in the smspp port of
  the vcpkg registry

- SatellitesBlock, the `Block` of the Satellite Constellation Design
  Problem, in its continuous and discrete versions, with the `Solver` of
  their sub-problems (`BUILD_SatellitesBlock`), and its feature
  `satellites` in the smspp port of the vcpkg registry

- SATBlock, the `Block` of the satisfiability problems and of the weighted
  partial MaxSAT, with the `SATSolver` that hand them to CaDiCaL and to
  MiniSat (`BUILD_SATBlock`); `INSTALL.sh` installs CaDiCaL from its package
  where there is one and from its source otherwise, and MiniSat from its
  source, whose library no distribution packages (`--without-cadical`,
  `--without-minisat`), Homebrew both on macOS; `extlib` has
  `makefile-libCaDiCaL` and `makefile-libMiniSat` and their default paths

- `INSTALL.sh` checks out the source of RECORD (`--without-record`), which
  BinaryKnapsackBlock compiles into `RECORDBinaryKnapsackSolver` when it finds
  it, and `extlib` has the default paths of RECORD and COMBO (`RECORD_ROOT`,
  `COMBO_ROOT`); COMBO, for academic or non-commercial use only, is not
  installed and is built only from a directory with its sources

- a commit says what it changes with the `Changelog:` and `Changelog-entry:`
  trailers of its message, or that it changes nothing worth telling with
  `[skip changelog]`; `changelog coverage` reads them, so that a commit is
  covered because it says so and not because an entry happens to share its
  words, and `changelog draft` collects them into the draft of a release
  [see CONTRIBUTING.md]

- `changelog`, which reads the CHANGELOG of the project and of every module:
  `check` says what does not hold in them, from a date that is not ISO to a
  link reference that names another release; `coverage` says which commits
  since the last tag no entry covers, so that what is missing is seen before
  the release and not after it; `links` writes the link references from the
  releases, each comparing against the release below it that has a tag; and
  `draft` writes the draft of a release, module by module, to be pruned. With
  `--repo` it works on one repository alone, which is what a module runs in
  its own pipeline

- the pipeline holds the CHANGELOG of the umbrella to the format at every
  push, reports on the whole project on a schedule, and writes the draft of
  the changelog among the artifacts of a release

- `ci/changelog.yml`, which every module includes to get the same job on its
  own CHANGELOG: the format at every push, and, on a merge request, an entry
  of [Unreleased] for every commit of the branch, unless its message carries
  [skip changelog]

### Changed

- every module is compiled as C++20, which the core already required, by
  CMake and by the makefiles alike, and so is a module generated from the
  ModuleTemplate; ACNetworkBlock and IntegralityBarrierFunction take pi from
  `std::numbers` rather than from the non-standard `M_PI` or a macro of their
  own

- upload-to-package-registry takes the version to publish, which the
  data/upload-* script of a module reads from its CMakeLists.txt: it refuses
  a version that is already there, a published version being never
  replaced, and writes `latest` too, for the trees that do not name a
  version yet

- `mirror-to-github` skips `moduletemplate`, which has moved to the private
  `smspp-develop` group and is not mirrored

- `INSTALL.sh` and `CMakeSettings.txt` build in Release when no build type is
  given, a build with no type carrying no optimization at all, which made
  everything built by hand one or two orders of magnitude slower than it had
  to be

- `INSTALL.sh` bounds the parallel jobs by the memory of the machine as well
  as by the cores, a compilation of the heavier headers taking more than a
  gigabyte per job and a machine with many cores and little memory going to
  the swap or being killed

### Removed

- COIN-OR (CoinUtils, Osi, Clp) and NDOSolver/FiOracle, which the
  BundleSolver no longer uses since its version 2.0: `INSTALL.sh` and
  `INSTALL.ps1` no longer build or look for them (`--without-coinor` is
  still accepted, and does nothing), and they are gone from `vcpkg.json`,
  from the `extlib` makefile paths, from the inputs of Doxygen and from the
  installation pages of the README and of the manual

### Fixed

- `CMakeSettings.txt` gives `-Wno-undefined-internal` to clang alone: it
  gave it to gcc too, which answered that it does not know the option and
  stopped there

- `changelog` takes the last release of a repository from its version tags
  alone: a tag that names something else, e.g., `archive/<branch>`, sorted
  above them, so `check` compared the CHANGELOG of `tests` with it and
  `coverage` and `draft` started the range of commits from it

## [0.6.3] - 2026-09-14

### Fixed

- the makefiles of the modules declare the same dependencies as CMake does:
  LagrangianDualSolver brings in MILPSolver, which PrimalProximalHeur needs,
  StochasticBlock no longer brings in the facility location Block, and the
  complete makefile of InvestmentBlock does not list the core objects twice

- StochasticBlock no longer turns CapacitatedFacilityLocationBlock on, which
  it stopped using when `ScenarioReductor.cpp` went away: a build of
  SDDPBlock, or of any other stochastic module, no longer drags in the
  facility location Block and the two Blocks it needs

## [0.6.2] - 2026-09-14

### Fixed

- the link that carries the name a tool had before the prefix is made in the
  directory of the install and not in the one of the configure, so that
  `cmake --install --prefix` puts it next to the tool instead of failing on
  the directory of the machine

## [0.6.1] - 2026-09-13

### Added

- the operating rules of a nuclear unit in the `NuclearUnitBlock` of UCBlock,
  with the labelled dynamic programming that solves them, and the options
  that decide whether the optimal schedule uses them

- the samples of a sparse data set are read as the list of their nonzeroes in
  SVMBlock, which is how its kernel is computed there: on `w8a`, which is 3.9
  per cent full, a factor of 4.5 on the solve

### Changed

- the installed tools carry the name of the project, e.g.
  `smspp_ucblock_solver` and `smspp_chgcfg`, so that they are recognisable
  among all the others where they are installed; each of them is also
  installed under the name it had before, which is a link to it and which a
  later release will drop

- `intLogVerb` of the `PrimalProximalHeur` of LagrangianDualSolver is one
  composite value, v + 4 * w, carrying the verbosity of the heuristic and
  that of its inner Solver

- the start of the Gurobi environment is retried, with a growing wait, when
  the license service refuses it for a transient reason

- the project is packaged for Ubuntu in the PPA `ppa:smspp/ppa`, one package
  per module and one per tool, and for Homebrew in the tap
  `SMSpp-Project/smspp`; the README and the wiki tell the four ways of
  installing it ready-made

### Fixed

- a direction is checked against a quadratic row of a `Block` too, the sign
  of the quadratic form deciding whether the row bounds it

- `FindStOpt` looks for the library in lowercase too, as a distribution
  packaging StOpt names it

- the master of the Benders decomposition needs the least numerical care of
  GUROBI, not none of it, and the residual of a cut is declared

- `is_sol_feasible()` and `is_sol_optimal()` of MCFBlock and of
  SingleFlowDCRBlock read the Solution and leave the Variable alone

## [0.6.0] - 2026-09-12

### Added

- MultiStageStochasticBlock, SVMBlock, SingleFlowDCRBlock, BranchAndXSolver,
  BendersDecompositionSolver, ScenarioReductionSolver, FrankWolfeSolver,
  MCFClassSolver and MCFLemonSolver among the submodules, and pySMSpp, the
  Python interface

- PIPS-IPM++, Torch, LIBSVM and LIBLINEAR among the optional dependencies,
  in INSTALL.sh, extlib and the vcpkg manifest

- the release of a tag puts its tarball in the generic package registry of
  the project, with the version of every submodule written in its
  VERSION.txt, and updates the smspp port of vcpkg-registry to it; the
  release notes list the submodules that are not on a tag of their own

### Changed

- every module is on a release of its own, whose version its library
  carries:

    BendersDecompositionSolver        0.1.0
    BinaryKnapsackBlock               0.4.0
    BranchAndXSolver                  0.1.0
    BundleSolver                      0.5.0
    CapacitatedFacilityLocationBlock  0.3.0
    FrankWolfeSolver                  0.2.0
    InvestmentBlock                   0.2.0
    LagrangianDualSolver              0.3.0
    LukFiBlock                        0.4.0
    MCFBlock                          0.6.0
    MCFClassSolver                    0.2.0
    MCFLemonSolver                    0.2.0
    MILPSolver                        0.9.0
    MMCFBlock                         0.4.0
    MultiStageStochasticBlock         0.1.0
    ScenarioReductionSolver           0.2.0
    SDDPBlock                         0.6.0
    SingleFlowDCRBlock                0.1.0
    SMS++                             0.7.0
    StochasticBlock                   0.7.0
    SVMBlock                          0.1.0
    tools                             0.6.0
    TwoStageStochasticBlock           0.2.0
    UCBlock                           0.8.0

- the version of the project, as that of every module, is its git tag

- LEMONSolver is MCFLemonSolver, and BUILD_LagrangianDualSolver turns on
  BUILD_MILPSolver

## [0.5.0] - 2025-12-12

### Added

- TwoStageStochasticBlock

- CHANGELOG.md

### Changed

- moved to X.Y.Z release names and started overdue changelog

- significant changes in installation scripts and cmake / makefiles

[Unreleased]: https://gitlab.com/smspp/smspp-project/-/compare/0.7.0...develop
[0.7.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.3...0.7.0
[0.6.3]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.2...0.6.3
[0.6.2]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.1...0.6.2
[0.6.1]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.0...0.6.1
[0.6.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.5.0...0.6.0
[0.5.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.4.0...0.5.0
