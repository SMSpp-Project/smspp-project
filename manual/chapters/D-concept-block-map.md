# [Appendix D](D-concept-block-map.md#app-D) — Concept-to-Block map {#app-D}

The manual threads three running `Block`s through the whole of Part II
and Part III, using whichever one shows a given concept most clearly:
`MCFBlock` (a leaf `Block` with a wrapper-style specialised `Solver`),
`BinaryKnapsackBlock` (a leaf `Block` with a *native* specialised
`Solver`), and `CapacitatedFacilityLocationBlock` (a non-leaf `Block`
with four formulations, a non-trivial `R3Block`, and a hidden
`BendersBFunction`). This appendix is the cross-index: for each
concept, it names the `Block` (or other class) that carries the worked
example and the section(s) where the example is developed. It is a
navigation aid, not a substitute for the chapters themselves.

| Concept | Illustrated mainly by | Section(s) |
|---|---|---|
| `Block` tree, the identity-is-address rule | `MCFBlock` | [§4](04-block.md#ch-4) |
| `Variable` / `Constraint` / `Objective` | `BinaryKnapsackBlock` | [§5](05-variable-constraint-objective.md#ch-5) |
| Specialised `Solver` on a leaf `Block` (wrapper) | `MCFSolver` | [§6](06-solver.md#ch-6) |
| Specialised `Solver` on a leaf `Block` (native) | `DPBinaryKnapsackSolver` | [§6](06-solver.md#ch-6), [R2](R2-knapsack-reopt.md#rec-R2) |
| `is_feasible` / `is_dual_feasible` / `is_optimal` | `MCFBlock` | [§6](06-solver.md#ch-6), [§7](07-physical-abstract.md#ch-7) |
| Physical vs abstract representation | `MCFBlock`, `BinaryKnapsackBlock` | [§7](07-physical-abstract.md#ch-7) |
| `Modification`, abstract `Modification`, the Janus discipline | `MCFBlock`, `BinaryKnapsackBlock` | [§8](08-modification-janus.md#ch-8) |
| `Solution` (`Block`-specific, physical and abstract) | `MCFSolution`, `BinaryKnapsackSolution` | [§9](09-solution.md#ch-9) |
| `R3Block` (non-trivial reformulation) | CFL → `MCFBlock` flow relaxation | [§10](10-r3block.md#ch-10), [R3](R3-cfl-three-ways.md#rec-R3) |
| Keeping an `R3Block` in sync (`UpdateSolver`) | CFL → `MCFBlock` flow relaxation | [§10.6](10-r3block.md#sec-10-6), [R3](R3-cfl-three-ways.md#rec-R3) |
| `Configuration` tree, `[C/O/R]BlockConfig` | CFL formulations | [§11](11-configuration.md#ch-11) |
| Sub-`Block` composition and recursive `Modification` flow | CFL / KskForm with `BinaryKnapsackBlock` sub-`Block`s | [§12](12-sub-block.md#ch-12), [R4](R4-cfl-lagrangian.md#rec-R4) |
| The `Function` family (C05/C15, linear / quadratic / polyhedral) | `MCFBlock`, `BinaryKnapsackBlock` | [§13](13-function-family.md#ch-13) |
| `LagBFunction` wrapping a `Block` | CFL / KskForm | [§14](14-lag-benders-bfunction.md#ch-14), [R4](R4-cfl-lagrangian.md#rec-R4) |
| `BendersBFunction` wrapping a `Block` | CFL / BenForm | [§14](14-lag-benders-bfunction.md#ch-14), [R5](R5-cfl-benders.md#rec-R5) |
| The methods factory | `BinaryKnapsackBlock::chg_weights` | [§15](15-methods-factory.md#ch-15), [R2](R2-knapsack-reopt.md#rec-R2) |
| `Change` (*beta*) | `BinaryKnapsackBlockChange` | [§16](16-change.md#ch-16), [R2](R2-knapsack-reopt.md#rec-R2) |
| Parallel / asynchronous computation (`lock`, `compute_async`, `State`) | CFL / KskForm with a parallel `BundleSolver` | [§17](17-parallel.md#ch-17) |
| Factories and netCDF [de]serialisation | all three `Block`s | [§18](18-factories-netcdf.md#ch-18), [R3](R3-cfl-three-ways.md#rec-R3) |
| `LagrangianDualSolver` | CFL / KskForm | [R4](R4-cfl-lagrangian.md#rec-R4) |
| `PrimalProximalHeur` | CFL / KskForm | [R4](R4-cfl-lagrangian.md#rec-R4) |
| Benders cuts via a user-cut callback | CFL / BenForm | [R5](R5-cfl-benders.md#rec-R5) |
| `BendersDecompositionSolver` | CFL / BenForm | [§14.3](14-lag-benders-bfunction.md#sec-14-3), [R5](R5-cfl-benders.md#rec-R5) |
| Writing a new `:Block`, `:Solver`, `:Modification` / `:Change` from scratch | `BinPackingBlock` (pedagogical) | Appendices [A](A-writing-block.md#app-A), [B](B-writing-solver.md#app-B), [C](C-writing-modification.md#app-C) |

Read the table in either direction: a reader following a concept can
jump to the `Block` that exercises it, and a reader interested in one
of the three `Block`s can collect, by scanning the middle column, every
place that `Block` appears in the manual.

## D.1 The modules of the distribution {#sec-D-1}

The concepts above are not confined to the three running `Block`s:
every module of the umbrella project ([§2.2](02-installation.md#sec-2-2)) exercises some of
them, and its sources are a further worked example of each. For the
reader who wants one, the table lists the modules of the umbrella
with the `:Block`s and the `:Solver`s they register in the factories
([Chapter 18](18-factories-netcdf.md#ch-18)); the classes of the core library are in the first
row, and the abstract bases of a module (e.g., `UnitBlock`,
`NetworkBlock`, `SATSolver`) are left out.

| Module | `:Block`s | `:Solver`s |
|------|----------|----------|
| `SMS++` (core) | `AbstractBlock`, `PolyhedralFunctionBlock`, `MasterProblemBlock`, `BendersBlock` | `UpdateSolver`, `BoxSolver`, `FakeSolver` |
| `MCFBlock` | `MCFBlock` | |
| `MCFClassSolver` | | `MCFSolver` (a template over the `MCFClass` solvers) |
| `MCFLemonSolver` | | `MCFLemonSolver` |
| `MMCFBlock` | `MMCFBlock`, `MMCFNetworkDesignBlock` | |
| `BinaryKnapsackBlock` | `BinaryKnapsackBlock` | `DPBinaryKnapsackSolver`, `CoreDPBinaryKnapsackSolver`, `COMBOBinaryKnapsackSolver`, `RECORDBinaryKnapsackSolver`, `GreedyRelaxationBinaryKnapsackSolver` |
| `MultiKnapsackAssignBlock` | `MultiKnapsackAssignBlock` | |
| `CapacitatedFacilityLocationBlock` | `CapacitatedFacilityLocationBlock` | |
| `UCBlock` | `UCBlock`, `ThermalUnitBlock`, `NuclearUnitBlock`, `HydroUnitBlock`, `HydroSystemUnitBlock`, `BatteryUnitBlock`, `IntermittentUnitBlock`, `SlackUnitBlock`, `ConversionUnitBlock`, `DCNetworkBlock`, `ACNetworkBlock`, `OTSNetworkBlock`, `ECNetworkBlock`, `DesignNetworkBlock` | `ThermalUnitDPSolver`, `ThermalUnitExtDPSolver`, `NuclearUnitExtDPSolver` |
| `InvestmentBlock` | `InvestmentBlock`, `InvestmentFunction` (also a `C05Function`) | |
| `StochasticBlock` | `StochasticBlock`, `ScenarioReductionBlock` | |
| `TwoStageStochasticBlock` | `TwoStageStochasticBlock` | |
| `MultiStageStochasticBlock` | `MultiStageStochasticBlock` | |
| `SDDPBlock` | `SDDPBlock` | `SDDPSolver`, `ParallelSDDPSolver`, `SDDPGreedySolver`, `SDDPSimulationSolver` |
| `ScenarioReductionSolver` | | `ScenarioReductionSolver`, `CSSCScenarioReductionSolver` |
| `LukFiBlock` | `LukFiBlock` | |
| `SVMBlock` | `SVMBlock`, `SVCBlock`, `SVRBlock` | `SMOSolver`, `LIBSVMSolver`, `LIBLINEARSolver` |
| `SingleFlowDCRBlock` | `SingleFlowDCRBlock`, `MultiFlowDCRBlock` | `SingleFlowDCRBendersSolver` |
| `SATBlock` | `SATBlock` | `CaDiCaLSATSolver`, `MiniSATSolver` |
| `SatellitesBlock` | `SatelliteBlock`, `SingleTargetBlock`, `MultiTargetBlock`, `MultiTargetBlockv2`, `ConstellationBlock`, `DiscreteSatelliteBlock`, `DiscreteConstellationBlock` | `SatelliteSolver`, `DiscreteSatelliteSolver` |
| `HQPBlock` | `HQPBlock`, `HQPDistBlock` | `HQPSolver`, `HQPStageSolver` |
| `MILPSolver` | | `CPXMILPSolver`, `GRBMILPSolver`, `SCIPMILPSolver`, `HiGHSMILPSolver`, `MOSEKMILPSolver`, `ClarabelMILPSolver`, `DAQPMILPSolver`, `HPIPMMILPSolver`, `OSQPMILPSolver`, `PERMONMILPSolver`, `PIPSMILPSolver`, `PIQPMILPSolver`, `ProxQPMILPSolver`, `QPMADMILPSolver`, `QPOASESMILPSolver`, `QuadProgMILPSolver` |
| `BundleSolver` | | `BundleSolver`, `ParallelBundleSolver`, `BundleSolverML` |
| `BDMinQuadSolver` | | `BDMinQuadSolver` (a `:Solver` of the `MasterProblemBlock`) |
| `SubGradSolver` | | `SubGradSolver` |
| `FrankWolfeSolver` | | `FrankWolfeSolver`, `IntegralityBarrierSolver` |
| `LagrangianDualSolver` | | `LagrangianDualSolver`, `PrimalProximalHeur`, `LagrangianDualRelaxationSolver`, `LagrangianDualRelaxationSolverML` |
| `BendersDecompositionSolver` | | `BendersDecompositionSolver` |
| `BranchAndXSolver` | | `BranchAndXSolver` |
