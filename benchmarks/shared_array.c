// shared_array.c
// Two or more threads repeatedly read/write a shared array.
// Usage: ./shared_array <num_threads> <array_mb> <duration_seconds>
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
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
clock_gettime(CLOCK_MONOTONIC, &start);
while (1) {
clock_gettime(CLOCK_MONOTONIC, &now);
double elapsed = (now.tv_sec - start.tv_sec)
+ (now.tv_nsec - start.tv_nsec) / 1e9;
if (elapsed >= a->duration_sec) break;
// Stride access pattern to stress LLC
for (size_t i = 0; i < a->len; i += CACHE_LINE / sizeof(uint64_t)) {
a->array[i] += (uint64_t)a->thread_id; // read + write
ops++;
}
}
a->ops_completed = ops;
return NULL;

}
int main(int argc, char *argv[])
{
if (argc != 4) {
fprintf(stderr, "Usage: %s <threads> <array_mb> <seconds>\n",
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
for (int i = 0; i < nthreads; i++) {
args[i] = (thread_arg_t){ .array = arr, .len = len,
.thread_id = i,
.duration_sec = duration };
pthread_create(&tids[i], NULL, worker, &args[i]);
}
uint64_t total_ops = 0;
for (int i = 0; i < nthreads; i++) {
pthread_join(tids[i], NULL);
total_ops += args[i].ops_completed;
}
printf("threads=%d array_mb=%zu duration=%ds total_ops=%lu\n",
nthreads, array_mb, duration, total_ops);
free(tids); free(args); free((void *)arr);
return 0;
}