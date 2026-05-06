# test/TwoStageStochasticBlock

A tester for `TwoStageStochasticBlock`, the SMS++ Block that wraps a
deterministic-equivalent two-stage stochastic program around an inner Block
(typically a `UCBlock`) cloned once per scenario, with a
`DiscreteScenarioSet` attached and a set of "here-and-now"
non-anticipativity constraints linking the first-stage variables across
scenarios.

The test layout mirrors `tests/LagrangianDualSolver_UC`: a single
`BlockSolverConfig` registers two Solver to the outer
`TwoStageStochasticBlock` — a `:MILPSolver` on the deterministic
equivalent and a `LagrangianDualSolver` using `BundleSolver` as the
inner Solver — and the executable performs both:

- **solver-vs-solver**: the two Solver are compared against each other
  to within `1e-5` relative tolerance;
- **solver-vs-FO**: when a reference objective value is provided on the
  command line, the first Solver's result is compared against it, with
  a configurable relative tolerance (default `1e-2`).

The usage of the executable is the following:

       ./TSSB_test TSSB-file [BSC-file ws ref tol]
       BSC-file: BlockSolverConfig description [BSPar.txt]
       ws:  0 = LagrangianDualSolver, 1 = PrimalProximalHeur [0]
            (the `1` slot is currently a placeholder kept for
            symmetry with tests/LagrangianDualSolver_UC/test.cpp)
       ref: reference objective value to compare against [none]
       tol: relative tolerance for the comparison [1e-2]

The reference values used by `batches/batch-ec` come from
`EnergyCommunity.jl@stochastic` solved on the same YAMLs from which
`UCBlock/tools/csv2netCDF/csv2nc4.jl` produced the `.nc4` files. The
"no thermal" and "no asset" variants are obtained by passing
`--no-thermal` / `--no-asset` to the `test_instance_with_EC_jl.jl`
harness, which fixes the corresponding first-stage `x_us` decisions
to 0.

The default tolerance (`1e-2`) is intentionally looser than the
solver-vs-solver tolerance (`1e-5`): the
`EnergyCommunity.jl@stochastic` numerical pipeline differs in scenario
sampling, MILP gap, scaling and presolve from the SMS++ one, so an
exact match is not expected. The current observed differences are
under 0.8% for all instances.

A makefile is also provided that builds the executable including the
`TwoStageStochasticBlock`, `UCBlock`, `LagrangianDualSolver`,
`BundleSolver`, `MILPSolver` modules and the core SMS++ library.

## Configuration files

- `BSPar-2S.txt` — outer config registering `:MILPSolver` (default
  `GRBMILPSolver`) + `LagrangianDualSolver`. The `LagrangianDualSolver`
  parameters mirror those used by `tools/tssb_solver/config/TSSBSCfg-LD.txt`
  (`intPushCostToOwner=1`, `intDoEasy=1`, `dbltStar=-1`, etc.) so that the
  Lagrangian relaxation behaves the same way as in the standalone solver.
- `BSPar-2S-EASY.txt` — same as `BSPar-2S.txt` but with `intDoEasy=0`
  (no "easy" components), used for the no-asset (NA) instances where
  every sub-Block is an `ECNetworkBlock` and BundleSolver would otherwise
  raise "no non-ECNetworkBlock candidate block to set as a `hard`
  component".
- `LPBSCfg.txt` — `BlockSolverConfig` for the `LagBFunction` instances
  produced by `LagrangianDualSolver` (CPLEX with LP relaxation enabled).
- `BSCfg.txt` — alternative LP/QP `BlockSolverConfig` (HiGHS with IPM)
  that may be referenced from `LPBSCfg.txt` when a deterministic LP
  oracle is required.
