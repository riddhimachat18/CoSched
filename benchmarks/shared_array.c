// shared_array.c
// Two or more threads repeatedly read/write a shared array.
// Usage: ./shared_array &lt;num_threads&gt; &lt;array_mb&gt; &lt;duration_seconds&gt;
#define _GNU_SOURCE
#include &lt;pthread.h&gt;
#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;
#include &lt;time.h&gt;
#include &lt;stdint.h&gt;
#define CACHE_LINE 64
typedef struct {
volatile uint64_t *array;
size_t len;
int thread_id;
int duration_sec;
uint64_t ops_completed;
} thread_arg_t;
void *worker(void *arg)
{
thread_arg_t *a = (thread_arg_t *)arg;
struct timespec start, now;
uint64_t ops = 0;
clock_gettime(CLOCK_MONOTONIC, &amp;start);
while (1) {
clock_gettime(CLOCK_MONOTONIC, &amp;now);
double elapsed = (now.tv_sec - start.tv_sec)
+ (now.tv_nsec - start.tv_nsec) / 1e9;
if (elapsed &gt;= a-&gt;duration_sec) break;
// Stride access pattern to stress LLC
for (size_t i = 0; i &lt; a-&gt;len; i += CACHE_LINE / sizeof(uint64_t)) {
a-&gt;array[i] += (uint64_t)a-&gt;thread_id; // read + write
ops++;
}
}
a-&gt;ops_completed = ops;
return NULL;

}
int main(int argc, char *argv[])
{
if (argc != 4) {
fprintf(stderr, &quot;Usage: %s &lt;threads&gt; &lt;array_mb&gt; &lt;seconds&gt;\n&quot;,
argv[0]);
return 1;
}
int nthreads = atoi(argv[1]);
size_t array_mb = atoi(argv[2]);
int duration = atoi(argv[3]);
size_t len = (array_mb * 1024 * 1024) / sizeof(uint64_t);
volatile uint64_t *arr = aligned_alloc(CACHE_LINE, len *
sizeof(uint64_t));
memset((void *)arr, 0, len * sizeof(uint64_t));
pthread_t *tids = malloc(nthreads * sizeof(pthread_t));
thread_arg_t *args = malloc(nthreads * sizeof(thread_arg_t));
for (int i = 0; i &lt; nthreads; i++) {
args[i] = (thread_arg_t){ .array = arr, .len = len,
.thread_id = i,
.duration_sec = duration };
pthread_create(&amp;tids[i], NULL, worker, &amp;args[i]);
}
uint64_t total_ops = 0;
for (int i = 0; i &lt; nthreads; i++) {
pthread_join(tids[i], NULL);
total_ops += args[i].ops_completed;
}
printf(&quot;threads=%d array_mb=%zu duration=%ds total_ops=%lu\n&quot;,
nthreads, array_mb, duration, total_ops);
free(tids); free(args); free((void *)arr);
return 0;
}