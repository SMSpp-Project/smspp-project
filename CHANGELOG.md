# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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

- upload-to-package-registry takes the version to publish, which the
  data/upload-* script of a module reads from its CMakeLists.txt: it refuses
  a version that is already there, a published version being never
  replaced, and writes `latest` too, for the trees that do not name a
  version yet

- `mirror-to-github` skips `moduletemplate`, which has moved to the private
  `smspp-develop` group and is not mirrored

- `CMakeSettings.txt` gives `-Wno-undefined-internal` to clang alone, gcc
  answering that it does not know the option and stopping there

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

[Unreleased]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.3...develop
[0.6.3]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.2...0.6.3
[0.6.2]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.1...0.6.2
[0.6.1]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.0...0.6.1
[0.6.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.5.0...0.6.0
[0.5.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.4.0...0.5.0
