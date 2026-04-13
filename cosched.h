// cosched.h
#ifndef COSCHED_H
#define COSCHED_H
#define COSCHED_VERSION 1
#define MAX_CPUS 512
#define LLC_DOMAIN_SIZE 4 // CPUs per LLC cluster (update for your
hardware)
#define VRUNTIME_THRESHOLD_NS 5000000ULL // 5 milliseconds
// Exit info for sched_ext
#include &lt;scx/common.bpf.h&gt;
UEI_DEFINE(uei);
#endif // COSCHED_H