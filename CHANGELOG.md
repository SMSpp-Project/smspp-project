# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

### Changed

### Fixed

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

[Unreleased]: https://gitlab.com/smspp/smspp-project/-/compare/0.6.0...develop
[0.6.0]: https://gitlab.com/smspp/smspp-project/-/compare/0.5.1...0.6.0
[0.5.0]: https://gitlab.com/smspp/smspp-project/-/tags/0.5.0
