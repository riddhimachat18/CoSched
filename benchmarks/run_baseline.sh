#!/bin/bash
# run_baseline.sh — runs shared_array under vanilla CFS
# Usage: sudo bash run_baseline.sh
# Output: baseline_results.csv
set -e
BINARY=./shared_array
ARRAY_MB=256
DURATION=30
RUNS=5

OUT=baseline_results.csv
# Verify CoSched is NOT loaded
if cat /sys/kernel/debug/sched_ext/state 2>/dev/null | grep -q 'enabled'; then
echo 'ERROR: sched_ext scheduler is active. Stop it first.'
exit 1
fi
echo 'scheduler,threads,run,llc_miss_count,llc_miss_rate_pct,ipc,wall_sec' > $OUT
for THREADS in 2 4 8 16; do
for RUN in $(seq 1 $RUNS); do
echo "CFS | threads=$THREADS | run=$RUN"
# Drop page cache to start clean
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
# Run with perf stat
PERF_OUT=$(sudo perf stat \
-e LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,instructions,cycles \
$BINARY $THREADS $ARRAY_MB $DURATION 2>&1)
# Parse perf output
LLC_LOADS=$(echo "$PERF_OUT" | grep 'LLC-loads' | grep -v miss | awk '{print $1}' | tr -d ',')
LLC_MISS=$(echo "$PERF_OUT" | grep 'LLC-load-misses' | awk '{print $1}' | tr -d ',')
INSTR=$(echo "$PERF_OUT" | grep 'instructions' | awk '{print $1}' | tr -d ',')
CYCLES=$(echo "$PERF_OUT" | grep 'cycles' | awk '{print $1}' | tr -d ',')
WALL=$(echo "$PERF_OUT" | grep 'seconds time elapsed' | awk '{print $1}')
MISS_RATE=$(echo "scale=4; $LLC_MISS / $LLC_LOADS * 100" | bc -l 2>/dev/null || echo 'N/A')
IPC=$(echo "scale=4; $INSTR / $CYCLES" | bc -l 2>/dev/null || echo 'N/A')
echo "CFS,$THREADS,$RUN,$LLC_MISS,$MISS_RATE,$IPC,$WALL" >> $OUT
done
done
echo "Baseline complete. Results in $OUT"