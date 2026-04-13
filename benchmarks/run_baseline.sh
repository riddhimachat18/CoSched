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
if cat /sys/kernel/debug/sched_ext/state 2&gt;/dev/null | grep -q &#39;enabled&#39;; then
echo &#39;ERROR: sched_ext scheduler is active. Stop it first.&#39;
exit 1
fi
echo &#39;scheduler,threads,run,llc_miss_count,llc_miss_rate_pct,ipc,wall_sec&#39; &gt;
$OUT
for THREADS in 2 4 8 16; do
for RUN in $(seq 1 $RUNS); do
echo &quot;CFS | threads=$THREADS | run=$RUN&quot;
# Drop page cache to start clean
sync &amp;&amp; echo 3 | sudo tee /proc/sys/vm/drop_caches &gt; /dev/null
# Run with perf stat
PERF_OUT=$(sudo perf stat \
-e LLC-loads,LLC-load-misses,LLC-stores,LLC-store-
misses,instructions,cycles \
$BINARY $THREADS $ARRAY_MB $DURATION 2&gt;&amp;1)
# Parse perf output
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
echo &quot;CFS,$THREADS,$RUN,$LLC_MISS,$MISS_RATE,$IPC,$WALL&quot; &gt;&gt; $OUT
done
done
echo &quot;Baseline complete. Results in $OUT&quot;