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
LOADER=../cosched # path to compiled CoSched loader
echo &#39;scheduler,threads,run,llc_miss_count,llc_miss_rate_pct,ipc,wall_sec&#39; &gt;
$OUT
for THREADS in 2 4 8 16; do
for RUN in $(seq 1 $RUNS); do
echo &quot;CoSched | threads=$THREADS | run=$RUN&quot;
# Start CoSched in background
sudo $LOADER &amp;
LOADER_PID=$!
sleep 1 # wait for scheduler to attach
# Verify it is loaded
cat /sys/kernel/debug/sched_ext/state
sync &amp;&amp; echo 3 | sudo tee /proc/sys/vm/drop_caches &gt; /dev/null
PERF_OUT=$(sudo perf stat \
-e LLC-loads,LLC-load-misses,LLC-stores,LLC-store-
misses,instructions,cycles \
$BINARY $THREADS $ARRAY_MB $DURATION 2&gt;&amp;1)
LLC_LOADS=$(echo &quot;$PERF_OUT&quot; | grep &#39;LLC-loads&#39; | grep -v miss | awk
&#39;{print $1}&#39; | tr -d &#39;,&#39;)
LLC_MISS=$(echo &quot;$PERF_OUT&quot; | grep &#39;LLC-load-misses&#39; | awk &#39;{print
$1}&#39; | tr -d &#39;,&#39;)
INSTR=$(echo &quot;$PERF_OUT&quot; | grep &#39;instructions&#39; | awk &#39;{print $1}&#39; |
tr -d &#39;,&#39;)
CYCLES=$(echo &quot;$PERF_OUT&quot; | grep &#39;cycles&#39; | awk &#39;{print $1}&#39; |
tr -d &#39;,&#39;)
WALL=$(echo &quot;$PERF_OUT&quot; | grep &#39;seconds time elapsed&#39; | awk
&#39;{print $1}&#39;)
MISS_RATE=$(echo &quot;scale=4; $LLC_MISS / $LLC_LOADS * 100&quot; | bc -l
2&gt;/dev/null || echo &#39;N/A&#39;)
IPC=$(echo &quot;scale=4; $INSTR / $CYCLES&quot; | bc -l
2&gt;/dev/null || echo &#39;N/A&#39;)
echo &quot;CoSched,$THREADS,$RUN,$LLC_MISS,$MISS_RATE,$IPC,$WALL&quot; &gt;&gt; $OUT
# Stop CoSched, restore CFS
sudo kill $LOADER_PID 2&gt;/dev/null
sleep 1
done
done
echo &quot;CoSched experiments complete. Results in $OUT&quot;