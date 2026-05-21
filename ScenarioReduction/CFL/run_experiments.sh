# default configuration, overridden by command-line args
WDIR=~/smspp-project/build/tests/ScenarioReduction/CFL
IDIR=~/smspp-project/CapacitatedFacilityLocationBlock/data/nc4/ORLib
INSTANCES="cap111"
N_VALUES="200"
K_VALUES="5"
METHODS="baseline dupacova bestfit firstfit"
#baseline, dupacova, bestfit, firstfit, cssc
SOLVERS="BSPar_CPLEX.txt"
#SOLVERS="BSPar_HiGHS.txt"
#SOLVERS="BSPar_GRB.txt"
#SOLVERS="BSPar_SCIP.txt"
SEED=3
OUTPUT_CSV="results.csv"

# parse command-line arguments 

usage() {
    echo "Usage: $0"
    echo ""
    echo "Options:"
    echo "  --instances  \"cap41 cap42 ...\"  Instances to run, or 'all' for all ORLib nc4 files"
    echo "  --n          \"50 100 200\"        Number of full scenarios"
    echo "  --k          \"5 10 20\"           Number of reduced scenarios"
    echo "  --methods    \"baseline cssc ...\" Reduction methods"
    echo "  --solver     BSPar_CPLEX.txt      Solver config file"
    echo "  --seed       42                   Random seed"
    echo "  --output     results.csv          Output CSV file"
    echo "  --help                            Show this message"
    echo ""
    echo "Examples:"
    echo "  $0 --instances \"cap41 cap42\" --n \"50 100\" --k \"5 10\" --methods \"cssc dupacova\""
    echo "  $0 --instances all --n 100 --k 10 --solver BSPar_HiGHS.txt"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --instances) INSTANCES="$2"; shift 2 ;;
        --n)         N_VALUES="$2";  shift 2 ;;
        --k)         K_VALUES="$2";  shift 2 ;;
        --methods)   METHODS="$2";   shift 2 ;;
        --solver)    SOLVERS="$2";   shift 2 ;;
        --seed)      SEED="$2";      shift 2 ;;
        --output)    OUTPUT_CSV="$2";shift 2 ;;
        --help)      usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
done

# resolve instance

cd "$WDIR" || { echo "Error: cannot cd to $WDIR"; exit 1; }

if [ "$INSTANCES" = "all" ]; then
    INSTANCES=""
    for f in "$IDIR"/*.nc4; do
        [ -f "$f" ] && INSTANCES="$INSTANCES $(basename "$f" .nc4)"
    done
    INSTANCES="${INSTANCES# }"
    echo "Auto-detected instances: $INSTANCES"
fi

for exe in CFLScenarioGenerator cfl_full_tss \
           cfl_cssc_test cfl_scenario_reduction_test; do
    [ ! -f "./$exe" ] && echo "Error: $exe not found" && exit 1
done

[ -n "$OUTPUT_CSV" ] && \
    echo "Instance,N,K,Method,Solver,Full_Obj,Reduced_Obj,Gap_Pct,Red_us,RedAlgo_us" \
    > "$OUTPUT_CSV"

printf "\n%-8s %-5s %-5s %-12s %-18s %14s %14s %8s %12s %14s\n" \
    "Instance" "N" "K" "Method" "Solver" \
    "Full obj" "Reduced obj" "Gap(%)" "Red(us)" "RedAlgo(us)"
printf "%s\n" "$(printf '=%.0s' {1..125})"

for SOLVER in $SOLVERS; do
    [ ! -f "$SOLVER" ] && echo "Warning: $SOLVER not found — skipping" && continue
    SOLVER_SHORT=$(basename "$SOLVER" .txt)

    for INSTANCE_NAME in $INSTANCES; do
        INSTANCE="$IDIR/${INSTANCE_NAME}.nc4"
        [ ! -f "$INSTANCE" ] && echo "Warning: $INSTANCE not found — skipping" && continue

        for N in $N_VALUES; do
            SCEN="../../ScenarioReduction/scenarios/CFL/${INSTANCE_NAME}_scenarios.nc4"

            # Step 1: generate scenarios (once per instance/N)
            ./CFLScenarioGenerator \
                -i "$INSTANCE" -n "$N" -s "$SEED" --verbose 0 2>/dev/null

            # Step 2: solve full TSS once per (instance, N)
            FULL_OUT=$(./cfl_full_tss \
                -i "$INSTANCE" -f "$SCEN" -n "$N" -c "$SOLVER" -v 1 2>/dev/null)
            FULL_OBJ=$(echo "$FULL_OUT" | grep "Objective" | awk '{print $NF}')

            # Step 3: run each reduction method with --skip-full
            for K in $K_VALUES; do
                [ "$K" -ge "$N" ] && continue

                for METHOD in $METHODS; do
                    if [ "$METHOD" = "cssc" ]; then
                        OUT=$(./cfl_cssc_test \
                            -i "$INSTANCE" -f "$SCEN" \
                            -n "$N" -k "$K" -c "$SOLVER" -v 1 --skip-full 2>/dev/null)
                        RED_OBJ=$(echo "$OUT"      | grep "Reduced :"        | awk '{print $NF}')
                        GAP=$(echo "$OUT"          | grep "Gap"              | grep -oP '\d+\.\d+(?=%)' | head -1)
                        RED_TIME=$(echo "$OUT"     | grep "Reduced solve"    | grep -oP '\d+(?= us)')
                        RED_ALGO_TIME=$(echo "$OUT"| grep "CSSC reduction"   | grep -oP '\d+(?= us)')
                    else
                        OUT=$(./cfl_scenario_reduction_test \
                            -i "$INSTANCE" -f "$SCEN" \
                            -n "$N" -r "$K" -m "$METHOD" -c "$SOLVER" -v 1 --skip-full 2>/dev/null)
                        RED_OBJ=$(echo "$OUT"      | grep "Reduced:"         | awk '{print $NF}')
                        GAP=$(echo "$OUT"          | grep "Diff\."           | grep -oP '\d+\.\d+(?=%)' | head -1)
                        RED_TIME=$(echo "$OUT"     | grep "Reduced solve t"  | grep -oP '\d+(?= us)')
                        RED_ALGO_TIME=$(echo "$OUT"| grep "Reduction time\." | grep -oP '\d+(?= us)')
                    fi

                    # compute gap from Full_Obj if not parsed
                    if [ -z "$GAP" ] && [ -n "$FULL_OBJ" ] && [ -n "$RED_OBJ" ]; then
                        GAP=$(awk "BEGIN {printf \"%.2f\", 100*sqrt(($RED_OBJ-$FULL_OBJ)^2)/$FULL_OBJ}")
                    fi

                    printf "%-8s %-5s %-5s %-12s %-18s %14s %14s %8s %12s %14s\n" \
                        "$INSTANCE_NAME" "$N" "$K" "$METHOD" "$SOLVER_SHORT" \
                        "$FULL_OBJ" "$RED_OBJ" "${GAP}%" \
                        "$RED_TIME" "$RED_ALGO_TIME"

                    [ -n "$OUTPUT_CSV" ] && \
                        echo "$INSTANCE_NAME,$N,$K,$METHOD,$SOLVER_SHORT,$FULL_OBJ,$RED_OBJ,$GAP,$RED_TIME,$RED_ALGO_TIME" \
                        >> "$OUTPUT_CSV"
                done
            done
        done
    done
done

printf "%s\n\n" "$(printf '=%.0s' {1..125})"
[ -n "$OUTPUT_CSV" ] && echo "Results saved to: $WDIR/$OUTPUT_CSV"