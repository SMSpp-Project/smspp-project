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
generated using the `CFLScenarioGenerator` tool and stored in the
`scenarios/CFL/` directory before running tests.

## Directory Structure

```
CFL/
├── scenarios/                  # Pre-generated scenario files
│   └── CFL/
│       ├── cap41_scenarios.nc4
│       ├── cap42_scenarios.nc4
│       └── ...
├── cfl_scenario_reduction_test # Heuristic reduction test executable
├── cfl_cssc_test               # CSSC reduction test executable
├── cfl_full_tss                # Full TSS solver executable
└── CFLScenarioGenerator        # Scenario generator and validator tool
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

### Step 1: Convert Instances to netCDF (once)

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
./CFLScenarioGenerator -i ../../../CapacitatedFacilityLocationBlock/data/nc4/ORLib/cap41.nc4

# The generator automatically saves to: scenarios/CFL/cap41_scenarios.nc4
```

Options:
- `-i`, `--instance`: Path to base CFL instance (required)
- `-n`, `--scenarios`: Number of scenarios to generate (default: 20)
- `-v`, `--variation`: Variation factor for demands (default: 0.2)
- `-s`, `--seed`: Random seed for reproducibility (default: 42)
- `-o`, `--output`: Output file path
- `--no-validate`: Skip feasibility validation
- `--validate-only`: Only validate instance, don't generate scenarios
- `--verbose`: Set verbosity level (0-2)
- `--timeout`: Timeout for validation (seconds)

### Step 3: Run Tests

#### Heuristic scenario reduction

```bash
# Run test with default settings
./cfl_scenario_reduction_test \
    -i ../../../CapacitatedFacilityLocationBlock/data/nc4/ORLib/cap41.nc4 \
    -n 20 -r 5 -m dupacova -c BSPar_CPLEX.txt -v 1

# The test will automatically load scenarios from: scenarios/CFL/cap41_scenarios.nc4
```

Available methods via `-m`: `baseline`, `dupacova`, `bestfit`, `firstfit`.

By default the executable solves both the full TSS (N scenarios) and the
reduced TSS (K scenarios) and prints a comparison. To skip the full TSS solve
(e.g. when `Full_Obj` is already known from `cfl_full_tss`), pass
`--skip-full`:

```bash
./cfl_scenario_reduction_test \
    -i cap41.nc4 -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 50 -r 5 -m dupacova -c BSPar_CPLEX.txt -v 1 --skip-full
```

#### CSSC scenario reduction

```bash
./cfl_cssc_test \
    -i ../../../CapacitatedFacilityLocationBlock/data/nc4/ORLib/cap41.nc4 \
    -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 20 -k 5 -c BSPar_CPLEX.txt -v 1
```

CSSC uses `-k` (not `-r`) for the number of reduced scenarios. It solves an
internal MILP, so it is slower than heuristic methods but typically achieves
a smaller optimality gap. The `--skip-full` flag is also supported:

```bash
./cfl_cssc_test \
    -i cap41.nc4 -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 50 -k 5 -c BSPar_CPLEX.txt -v 1 --skip-full
```

#### Full Two Stage Stochastic (reference baseline)

```bash
./cfl_full_tss \
    -i ../../../CapacitatedFacilityLocationBlock/data/nc4/ORLib/cap41.nc4 \
    -f scenarios/CFL/cap41_scenarios.nc4 \
    -n 20 -c BSPar_CPLEX.txt -v 1
```

## Batch Experiments

### run_experiments.sh

Runs each `(instance, N, K, method)` combination once. The script follows
this execution order for a fair and efficient comparison:

```
for each (instance, N):
    Step 1: CFLScenarioGenerator   generate scenario file once
    Step 2: cfl_full_tss           solve full TSS once → Full_Obj
    Step 3: for each (K, method):
                 cfl_*_test --skip-full   solve reduced TSS only
```

The full TSS is solved **once per (instance, N)** and its objective is reused
across all methods and K values. Each reduction method uses `--skip-full` to
avoid redundant full solves, ensuring a fair and consistent comparison.

```bash
cd <build-dir>/tests/ScenarioReduction/CFL
bash <source-dir>/tests/ScenarioReduction/CFL/run_experiments.sh
```

The script supports two usage modes. The configuration section at the top of
the file defines the default values:

```bash
INSTANCES="cap41 cap42"
N_VALUES="50 100"
K_VALUES="5 10"
METHODS="baseline dupacova bestfit firstfit cssc"
SOLVERS="BSPar_CPLEX.txt"
SEED=42
OUTPUT_CSV="results.csv"
```

These defaults can also be overridden directly from the terminal without
editing the file:

```bash
# Override specific parameters
./run_experiments.sh --instances "cap41" --n "50 100" --k "5 10" \
                     --methods "cssc dupacova" --solver BSPar_CPLEX.txt

# Run on all available ORLib instances
./run_experiments.sh --instances all --n 100 --k 10

# Show available options
./run_experiments.sh --help
```

Results are printed to the console and saved to `results.csv` with columns:
`Instance, N, K, Method, Solver, Full_Obj, Reduced_Obj, Gap_Pct, Red_us, RedAlgo_us`

## Usage Details

### Instance Validation

```bash
# Validate a single instance (using CFLScenarioGenerator in validate-only mode)
./CFLScenarioGenerator --instance instance.nc4 --validate-only

# Set time limit for validation
./CFLScenarioGenerator --instance instance.nc4 --validate-only --timeout 60 --verbose 2
```

The validator checks if CFL instances are feasible with single-sourcing
constraints.

### Single Instance Generation

```bash
./CFLScenarioGenerator -i instance.nc4 -n 100 -v 0.2 -o output.nc4
```

## Output Format

### Scenario files (CFLScenarioGenerator)

Generated scenarios are saved as `DiscreteScenarioSet` in netCDF format:

- Scenarios: 2D array of demand values
- Probabilities: Uniform probability distribution
- Metadata: Generator info, variation factor, timestamp

### Experiment results (run_experiments.sh)

Results are saved to a CSV file with one row per `(instance, N, K, method)`
combination:

- `Full_Obj`: objective value of the full TSS problem (N scenarios), solved
  once per (instance, N) by `cfl_full_tss` and shared across all methods
- `Reduced_Obj`: objective value of the reduced TSS problem (K scenarios)
- `Gap_Pct`: relative gap between full and reduced objectives (%)
- `Red_us`: time to solve the reduced TSS problem (µs)
- `RedAlgo_us`: time to run the reduction algorithm itself (µs)

## Scenario Characteristics

The generator creates four types of demand clusters:

- Normal: Small variations around original demands (±10%)
- High: Increased demand scenarios (1.3x–1.5x)
- Low: Decreased demand scenarios (0.5x–0.7x)
- Mixed: Regional variations (half high, half low)

## Timing Notes

All timing values are reported in **microseconds (µs)**:
- `Red_us`: time to solve the reduced TSS problem
- `RedAlgo_us`: time to run the reduction algorithm itself

For heuristic methods, `RedAlgo_us` is typically under 1000 µs. For CSSC,
`RedAlgo_us` can range from thousands to millions of µs depending on N and K,
since it involves solving an internal MILP subproblem.

## Integration

Generated scenarios can be used with:

- `cfl_scenario_reduction_test` for heuristic scenario reduction experiments
- `cfl_cssc_test` for CSSC scenario reduction experiments
- `cfl_full_tss` for full Two-Stage Stochastic CFL solving
- Any tool that accepts `DiscreteScenarioSet` format