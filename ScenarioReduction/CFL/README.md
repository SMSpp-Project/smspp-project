# CFL Scenario Reduction Test

This directory contains the CFL-specific implementation of the scenario
reduction test framework, along with tools for generating and managing
scenarios for Capacitated Facility Location (CFL) problems.

## Components

### Test Executables

- `cfl_scenario_reduction_test`: Main test executable for heuristic scenario
  reduction methods (Baseline, Dupacova, BestFit, FirstFit)
- `cfl_cssc_test`: Test executable for the CSSC (Consistent Scenario Subset
  Clustering) reduction algorithm
- `cfl_full_tss`: Solves the full Two-Stage Stochastic CFL without scenario
  reduction, used as reference baseline
- `CFLScenarioGenerator`: Standalone scenario generator and instance validator

### Source Files

- `CFLCSSCScenarioReductionTest.cpp`: CSSC algorithm entry point
- `CFLTwoStageStochasticTest.cpp`: Full TSS solver entry point
- `cfl_scenario_reduction_main.cpp`: Entry point for heuristic reduction methods
- `CFLScenarioReductionTest.{h,cpp}`: CFL-specific test implementation shared
  by all executables

### Configuration

- `BSPar_CPLEX.txt`: CPLEX solver parameters
- `BSPar_HiGHS.txt`: HiGHS solver parameters
- `BSPar_GRB.txt`: Gurobi solver parameters
- `BSPar_SCIP.txt`: SCIP solver parameters

## Important: Scenario Management

The test framework requires pre-generated scenarios. Scenarios must be
generated using the `CFLScenarioGenerator` tool before running any tests.
By default they are stored in the `scenarios/CFL/` directory under the build
folder.

## Directory Structure

```
tests/ScenarioReduction/CFL/
├── CFLCSSCScenarioReductionTest.cpp
├── CFLTwoStageStochasticTest.cpp
├── cfl_scenario_reduction_main.cpp
├── CFLScenarioReductionTest.h
├── CFLScenarioReductionTest.cpp
├── CFLScenarioGenerator.cpp
├── CMakeLists.txt
├── run_experiments.sh
├── BSPar_CPLEX.txt
├── BSPar_HiGHS.txt
├── BSPar_GRB.txt
└── BSPar_SCIP.txt
```

Scenario files and results are written to the build directory:

```
<build-dir>/tests/ScenarioReduction/CFL/
├── scenarios/CFL/          # Generated scenario files
└── results.csv             # Experiment results
```

## Building

All targets are defined in `CMakeLists.txt` and built through the SMS++ CMake
system:

```bash
cd <build-dir>
cmake ..
cmake --build . --target CFLScenarioGenerator \
                --target cfl_scenario_reduction_test \
                --target cfl_cssc_test \
                --target cfl_full_tss -j$(nproc)
```

## Workflow

### Step 1: Convert instances to netCDF (once)

Raw ORLib `.txt` instances must be converted to netCDF format before use.
Use the `txt2nc4` tool provided by `CapacitatedFacilityLocationBlock`:

```bash
# Convert a single instance
txt2nc4 cap41.txt 0 cap41.nc4

# Convert all ORLib instances at once
for f in <source-dir>/data/txt/ORLib/*.txt; do
    name=$(basename "$f" .txt)
    txt2nc4 "$f" 0 <source-dir>/data/nc4/ORLib/${name}.nc4
done
```

### Step 2: Generate Scenarios

Before running any tests, generate scenarios for your instances:

```bash
# Generate scenarios for a single instance
./CFLScenarioGenerator \
    -i ../../../CapacitatedFacilityLocationBlock/data/nc4/ORLib/cap41.nc4 \
    -n 100 -s 42

# The generator automatically saves to: scenarios/CFL/cap41_scenarios.nc4

# Specify a custom output path
./CFLScenarioGenerator \
    -i cap41.nc4 -n 100 -s 42 \
    -o scenarios/CFL/cap41_s42_scenarios.nc4
```

Options:
- `-i`, `--instance`: Path to base CFL instance file (required)
- `-n`, `--scenarios`: Number of scenarios to generate (default: 20)
- `-v`, `--variation`: Variation factor for demands (default: 0.2)
- `-s`, `--seed`: Random seed for reproducibility (default: 42)
- `-o`, `--output`: Output path for scenarios
- `--no-validate`: Skip feasibility validation
- `--validate-only`: Only validate instance, don't generate scenarios
- `--verbose`: Verbosity level 0–2 (default: 1)
- `--timeout`: Validation timeout per scenario in seconds (default: 10)

### Step 3: Run Tests

#### Heuristic scenario reduction

```bash
./cfl_scenario_reduction_test \
    -i cap41.nc4 \
    -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 50 -r 5 -m dupacova \
    -c BSPar_CPLEX.txt -v 1
```

Available methods via `-m`: `baseline`, `dupacova`, `bestfit`, `firstfit`.

#### CSSC scenario reduction

```bash
./cfl_cssc_test \
    -i cap41.nc4 \
    -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 50 -k 5 \
    -c BSPar_CPLEX.txt -v 1
```

CSSC uses `-k` (not `-r`) for the number of reduced scenarios. It solves an
internal MILP, so it is slower than heuristic methods but typically achieves
a smaller optimality gap.

#### Full TSS (reference baseline)

```bash
./cfl_full_tss \
    -i cap41.nc4 \
    -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 50 -c BSPar_CPLEX.txt -v 1
```

## Batch Experiments

### run_experiments.sh

Runs each `(instance, N, K, method)` combination once. The full TSS is solved
**once per (instance, N)** and reused across all methods for a fair comparison.

```bash
cd <build-dir>/tests/ScenarioReduction/CFL
bash <source-dir>/tests/ScenarioReduction/CFL/run_experiments.sh
```

The script supports two usage modes. The configuration section at the top of
the file defines the default values:

```bash
INSTANCES="cap41 "
N_VALUES="100"
K_VALUES="10"
METHODS="baseline dupacova bestfit firstfit cssc"
SOLVERS="BSPar_CPLEX.txt"
SEED=42
OUTPUT_CSV="results.csv"
```

These defaults can also be overridden directly from the terminal without
editing the file:

```bash
# Override specific parameters
./run_experiments.sh --instances "cap41 cap71 cap111" --n "50 100" --k "5 10" \
                     --methods "cssc dupacova" --solver BSPar_CPLEX.txt

# Run on all available ORLib instances
./run_experiments.sh --instances all --n 100 --k 10

# Show available options
./run_experiments.sh --help
```

Results are printed to the console and saved to `results.csv` with columns:
`Instance, N, K, Method, Solver, Full_Obj, Reduced_Obj, Gap_Pct, Red_us, RedAlgo_us`

## Timing Notes

All timing values are reported in **microseconds (µs)**:
- `Red_us`: time to solve the reduced TSS problem
- `RedAlgo_us`: time to run the reduction algorithm itself

For heuristic methods, `RedAlgo_us` is typically under 1000 µs. For CSSC,
`RedAlgo_us` can range from thousands to millions of µs depending on N and K,
since it involves solving an internal MILP subproblem.