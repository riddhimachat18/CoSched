#!/bin/bash
# run_cosched.sh — runs shared_array under CoSched
# Must be run AFTER baseline. Uses same parameters.
# Usage: sudo bash run_cosched.sh

set -e
BINARY=./shared_array
ARRAY_MB=256
DURATION=30
RUNS=5
OUT=cosched_results.csv
LOADER=/home/nilay-kumar/CoSched/cosched # path to compiled CoSched loader
echo 'scheduler,threads,run,llc_miss_count,llc_miss_rate_pct,ipc,wall_sec' > $OUT
for THREADS in 2 4 8 16; do
for RUN in $(seq 1 $RUNS); do
echo "CoSched | threads=$THREADS | run=$RUN"
# Start CoSched in background
sudo $LOADER &
LOADER_PID=$!
sleep 2 # wait for scheduler to attach
# Verify it is loaded
cat /sys/kernel/sched_ext/state
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
PERF_OUT=$(sudo perf stat \
-e LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,instructions,cycles \
$BINARY $THREADS $ARRAY_MB $DURATION 2>&1)
LLC_LOADS=$(echo "$PERF_OUT" | grep 'LLC-loads' | grep -v miss | awk '{print $1}' | tr -d ',')
LLC_MISS=$(echo "$PERF_OUT" | grep 'LLC-load-misses' | awk '{print $1}' | tr -d ',')
INSTR=$(echo "$PERF_OUT" | grep 'instructions' | awk '{print $1}' | tr -d ',')
CYCLES=$(echo "$PERF_OUT" | grep 'cycles' | awk '{print $1}' | tr -d ',')
WALL=$(echo "$PERF_OUT" | grep 'seconds time elapsed' | awk '{print $1}')
MISS_RATE=$(echo "scale=4; $LLC_MISS / $LLC_LOADS * 100" | bc -l 2>/dev/null || echo 'N/A')
IPC=$(echo "scale=4; $INSTR / $CYCLES" | bc -l 2>/dev/null || echo 'N/A')
echo "CoSched,$THREADS,$RUN,$LLC_MISS,$MISS_RATE,$IPC,$WALL" >> $OUT
# Check state after perf
cat /sys/kernel/sched_ext/state
# Stop CoSched, restore CFS
sudo kill $LOADER_PID 2>/dev/null
sleep 1
done
done
echo "CoSched experiments complete. Results in $OUT"