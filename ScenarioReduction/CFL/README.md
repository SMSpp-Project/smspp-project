# CFL Scenario Reduction Test

This directory contains the Capacitated Facility Location (CFL) test for
scenario reduction. A single self contained executable solves the full
two stage stochastic problem, runs a chosen scenario reduction method, solves
the reduced problem, and reports the in sample gap between them. A scenario
generator and a batch sweep script complete the workflow.

## Components

### Executables

- `cfl_scenario_reduction_test`: the main test. In ONE run it loads a CFL
  instance, loads a scenario set, solves the full two stage problem (N
  scenarios), applies a reduction method to pick K representatives, solves the
  reduced problem, and prints the objectives plus the in sample gap and the
  reduction time. All methods (including cssc) go through this one binary.
- `CFLScenarioGenerator`: standalone tool that generates a `DiscreteScenarioSet`
  of customer demand scenarios for a CFL instance.

### Source files

- `CFLScenarioReductionTest.cpp`: implementation of `cfl_scenario_reduction_test`
  (the standalone main, scenario loading, TSSB build, reduction, reporting).
- `CFLScenarioGenerator.cpp`: implementation of `CFLScenarioGenerator`.

Note: the generic, problem agnostic solvers live one level up in
`tests/ScenarioReduction/src` (`ScenarioReductionSolver` for the heuristics and
`CSSCScenarioReductionSolver` for CSSC). They are compiled into this test and
shared with the UC test.

### Configuration

Solver configuration files live in the parent directory
`tests/ScenarioReduction`:

- `BSPar_CPLEX.txt`: CPLEX solver parameters (default)
- `BSPar_HiGHS.txt`: HiGHS solver parameters
- `BSPar_GRB.txt`: Gurobi solver parameters
- `BSPar_SCIP.txt`: SCIP solver parameters

## What the test measures

The extensive form built here is a genuine two stage stochastic program: the
facility open decisions `y` are first stage (here and now) variables shared by
all scenarios through non anticipativity constraints, while the customer
assignment is the second stage (recourse) that adapts to each scenario demand.

The test reports:

- `Full_Obj`: optimum of the full problem over all N scenarios.
- `Reduced_Obj`: optimum of the reduced problem over the K selected scenarios.
- `Gap`: the in sample gap, `|Reduced_Obj - Full_Obj| / |Full_Obj|`.

The gap measures how close the reduced objective value is to the full one. It
is an in sample quantity, computed on the same K scenarios the decision was
built on.

## Reduction methods

Selected with `-m`:

- `baseline`: simple representative selection
- `dupacova`: Dupacova forward selection (greedy Kantorovich/Wasserstein)
- `bestfit`: best fit assignment heuristic
- `firstfit`: first fit assignment heuristic
- `cssc`: Consistent Scenario Subset Clustering (solves an internal MILP; slower
  than the heuristics but builds a cost aware clustering)

## Building

Targets are built through the SMS++ CMake system:

```bash
cd <build-dir>
cmake ..
cmake --build . --target cfl_scenario_reduction_test --target CFLScenarioGenerator -j$(nproc)
```

The binaries are produced under
`<build-dir>/tests/ScenarioReduction/CFL`.

## Workflow

### Step 1: convert an instance to netCDF (once)

Raw ORLib `.txt` instances must be converted to netCDF before use, with the
`txt2nc4` tool provided by `CapacitatedFacilityLocationBlock`:

```bash
txt2nc4 cap41.txt 0 cap41.nc4
```

### Step 2: generate scenarios

```bash
./CFLScenarioGenerator -i <instance.nc4> -o <scenarios.nc4> -n 100 -v 0.1 -s 42
```

Options:

- `-i`, `--instance`: base CFL instance (required)
- `-o`, `--output`: output scenario file
- `-n`, `--scenarios`: number of scenarios (default 20)
- `-v`, `--variation`: demand variation factor (default 0.2)
- `-s`, `--seed`: random seed
- `--no-validate`: skip the feasibility validation step (faster, fully
  deterministic for a given seed)

### Step 3: run one test

```bash
./cfl_scenario_reduction_test \
    -i <instance.nc4> -f <scenarios.nc4> \
    -n 100 -r 5 -m cssc -c ../BSPar_CPLEX.txt
```

Options:

- `-i`: CFL instance netCDF file (required)
- `-f`: scenario netCDF file (required)
- `-n`: limit the total number of scenarios to N (default: all in the file)
- `-r`: number of reduced scenarios K (default 5)
- `-m`: reduction method (default `dupacova`)
- `-c`: solver config file (default `BSPar_CPLEX.txt`)
- `-v`: verbosity (default 0)

Example output:

```
Full TSS  (N=100): 1.0301e+06  (11.45s)
Reduced TSS (K=5): 1.0234e+06  (0.24s)
Reduction time: 0.30s
Gap (absolute): 6785.5  (0.659%)
```

## Batch experiments

`run_cfl_tests.sh` sweeps over instances, N, K and methods, then prints an
aligned table and writes a CSV. It uses only `cfl_scenario_reduction_test`
(which already solves both the full and the reduced problem), and parses the
gap and timings straight from its output.

```bash
bash run_cfl_tests.sh
bash run_cfl_tests.sh --instances "cap41 cap121" --n "50 100" --k "5 10" \
     --methods "dupacova cssc" --solver BSPar_CPLEX.txt --seed 42
```

Flags: `--instances --n --k --methods --solver --seed --variation --output`.

The defaults are set in the configuration block at the top of the script.

## Output format

The table and the CSV have one row per `(instance, N, K, method)` with columns:

- `Full_Obj`: full problem objective (N scenarios)
- `Reduced_Obj`: reduced problem objective (K scenarios)
- `Gap_Pct`: in sample gap percentage
- `RedTime_s`: time to solve the reduced problem (seconds)
- `AlgoTime_s`: time of the reduction algorithm itself (seconds)

`AlgoTime_s` is the genuine computational cost of the reduction method. It is
tiny for the heuristics and large for cssc (which builds an N by N cost matrix
and solves a partitioning MILP).

## Scenario characteristics

`CFLScenarioGenerator` perturbs the base customer demands into clusters
(normal, high, low, mixed) controlled by the variation factor. With validation
enabled it checks single sourcing feasibility; with `--no-validate` it is fast
and fully reproducible for a fixed seed.

## Notes

- The CFL is solved in the unsplittable (single sourcing) version, so a too
  aggressive reduction can produce a first stage design that is infeasible or
  costly on the full scenario set.
- cssc is much slower than the heuristics. Use a modest N when including it.
- The in sample gap is not monotone in K and does not by itself rank methods by
  decision quality; it only measures the objective value approximation.
