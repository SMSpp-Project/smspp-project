#!/usr/bin/env bash
set -u

WDIR=~/smspp-project/build/tests/ScenarioReduction/CFL
IDIR=~/smspp-project/CapacitatedFacilityLocationBlock/data/nc4/ORLib
SDIR=/tmp/cfl_scenarios

INSTANCES="cap91"
N_VALUES="20 50 100"
K_VALUES="5 10"
METHODS="baseline dupacova bestfit firstfit cssc"
SOLVER="BSPar_CPLEX.txt"
SEED=42
VARIATION="0.6"
OUTPUT_CSV="insample_results_cap91_seed_42.csv"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --instances) INSTANCES="$2"; shift 2 ;;
        --n)         N_VALUES="$2";  shift 2 ;;
        --k)         K_VALUES="$2";  shift 2 ;;
        --methods)   METHODS="$2";   shift 2 ;;
        --solver)    SOLVER="$2";    shift 2 ;;
        --seed)      SEED="$2";      shift 2 ;;
        --variation) VARIATION="$2"; shift 2 ;;
        --output)    OUTPUT_CSV="$2";shift 2 ;;
        *) shift ;;
    esac
done

cd "$WDIR" || exit 1
mkdir -p "$SDIR"

MAXN=0; for n in $N_VALUES; do [ "$n" -gt "$MAXN" ] && MAXN="$n"; done

echo "Instance,N,K,Method,Full_Obj,Reduced_Obj,Gap_Pct,RedTime_s,AlgoTime_s" > "$OUTPUT_CSV"
printf "\n%-8s %-5s %-4s %-10s %16s %16s %10s %15s %15s\n" \
    "Instance" "N" "K" "Method" "Full_Obj" "Reduced_Obj" "Gap(%)" "RedTime(s)" "AlgoTime(s)"
printf '%.0s=' {1..106}; printf "\n"

for INST in $INSTANCES; do
    INSTANCE="$IDIR/${INST}.nc4"
    SCEN="$SDIR/${INST}_n${MAXN}_s${SEED}_v${VARIATION}.nc4"
    [ ! -f "$SCEN" ] && ./CFLScenarioGenerator -i "$INSTANCE" -o "$SCEN" -n "$MAXN" -s "$SEED" -v "$VARIATION" >/dev/null 2>&1

    for N in $N_VALUES; do
        for K in $K_VALUES; do
            [ "$K" -ge "$N" ] && continue
            for M in $METHODS; do
                OUT=$(./cfl_scenario_reduction_test -i "$INSTANCE" -f "$SCEN" -n "$N" -r "$K" -m "$M" -c "$SOLVER" 2>&1)

                FULL=$(echo "$OUT" | grep -oP 'Full TSS[^:]*:\s*\K[0-9.eE+-]+' | head -1)
                RED=$(echo "$OUT" | grep -oP 'Reduced TSS[^:]*:\s*\K[0-9.eE+-]+' | head -1)
                GAP=$(echo "$OUT" | grep -oP 'Gap \(absolute\):[^(]*\(\K[0-9.eE+-]+' | head -1)
                
                RTIME=$(echo "$OUT" | grep -oP 'Reduced TSS[^:]*:\s*[0-9.eE+-]+\s*\(\K[0-9.]+(?=s\))' | head -1)

                ALGOTIME=$(echo "$OUT" | grep -oP 'Reduction time:\s*\K[0-9.]+' | head -1)
                : ${FULL:=NA}; : ${RED:=NA}; : ${GAP:=NA}; : ${RTIME:=NA}; : ${ALGOTIME:=NA}

                printf "%-8s %-5s %-4s %-10s %16s %16s %10s %15s %15s\n" \
                    "$INST" "$N" "$K" "$M" "$FULL" "$RED" "$GAP" "$RTIME" "$ALGOTIME"
                echo "$INST,$N,$K,$M,$FULL,$RED,$GAP,$RTIME,$ALGOTIME" >> "$OUTPUT_CSV"
            done
        done
    done
done