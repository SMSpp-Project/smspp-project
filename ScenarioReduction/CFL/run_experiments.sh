WDIR=~/smspp-project/build/tests/ScenarioReduction/CFL
IDIR=~/smspp-project/CapacitatedFacilityLocationBlock/data/nc4/ORLib
INSTANCES="cap81"
N_VALUES="100"
K_VALUES="5"
METHODS="baseline dupacova bestfit firstfit cssc"
#baseline, dupacova, bestfit, firstfit, cssc
SOLVERS="BSPar_CPLEX.txt"
#SOLVERS="BSPar_HiGHS.txt"
#SOLVERS="BSPar_GRB.txt"
#SOLVERS="BSPar_SCIP.txt"

SEED=1
OUTPUT_CSV="results.csv"

# End of configuration

cd "$WDIR" || { echo "Error: cannot cd to $WDIR"; exit 1; }

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
printf "%s\n" "$(printf '=%.0s' {1..121})"

for SOLVER in $SOLVERS; do
    [ ! -f "$SOLVER" ] && echo "Warning: $SOLVER not found, skipping" && continue
    SOLVER_SHORT=$(basename "$SOLVER" .txt)

    for INSTANCE_NAME in $INSTANCES; do
        INSTANCE="$IDIR/${INSTANCE_NAME}.nc4"
        [ ! -f "$INSTANCE" ] && echo "Warning: $INSTANCE not found, skipping" && continue

        for N in $N_VALUES; do
            SCEN="../../ScenarioReduction/scenarios/CFL/${INSTANCE_NAME}_scenarios.nc4"

            # Step 1: generate scenarios (once per instance/N)
            ./CFLScenarioGenerator \
                -i "$INSTANCE" -n "$N" --seed "$SEED" --verbose 0 2>/dev/null
            # Step 2: solve full problem (once per instance/N/solver)
            FULL_OUT=$(./cfl_full_tss \
                -i "$INSTANCE" -f "$SCEN" -n "$N" -c "$SOLVER" -v 1 2>/dev/null)
            FULL_OBJ=$(echo "$FULL_OUT"  | grep "Objective" | awk '{print $NF}')
            FULL_TIME=$(echo "$FULL_OUT" | grep "Time"      | grep -oP '\d+(?= us)')

            # Step 3: run each reduction method, reuse FULL_OBJ / FULL_TIME
            for K in $K_VALUES; do
                [ "$K" -ge "$N" ] && continue

                for METHOD in $METHODS; do
                    if [ "$METHOD" = "cssc" ]; then
                        OUT=$(./cfl_cssc_test \
                            -i "$INSTANCE" -f "$SCEN" \
                            -n "$N" -k "$K" -c "$SOLVER" -v 1 2>/dev/null)
                        RED_OBJ=$(echo "$OUT"      | grep "Reduced :"       | awk '{print $NF}')
                        GAP=$(echo "$OUT"          | grep "Gap"             | grep -oP '\d+\.\d+(?=%)' | head -1)
                        RED_TIME=$(echo "$OUT"     | grep "Reduced solve"   | grep -oP '\d+(?= us)')
                        RED_ALGO_TIME=$(echo "$OUT"| grep "CSSC reduction"  | grep -oP '\d+(?= us)')
                    else
                        OUT=$(./cfl_scenario_reduction_test \
                            -i "$INSTANCE" -f "$SCEN" \
                            -n "$N" -r "$K" -m "$METHOD" -c "$SOLVER" -v 1 2>/dev/null)
                        RED_OBJ=$(echo "$OUT"      | grep "Reduced:"        | awk '{print $NF}')
                        GAP=$(echo "$OUT"          | grep "Diff\."          | grep -oP '\d+\.\d+(?=%)' | head -1)
                        RED_TIME=$(echo "$OUT"     | grep "Reduced solve t" | grep -oP '\d+(?= us)')
                        RED_ALGO_TIME=$(echo "$OUT"| grep "Reduction time\."| grep -oP '\d+(?= us)')
                        # note: Full_Obj from cfl_full_tss above,
                        # NOT from cfl_scenario_reduction_test's internal solve.
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

printf "%s\n\n" "$(printf '=%.0s' {1..121})"
[ -n "$OUTPUT_CSV" ] && echo "Results saved to: $WDIR/$OUTPUT_CSV"