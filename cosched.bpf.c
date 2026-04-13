// cosched.bpf.c - Cache-collocated scheduler using sched_ext
#include "vmlinux.h"
#include <scx/common.bpf.h>
#include "cosched.h"

char _license[] SEC("license") = "GPL";

#define VRUNTIME_THRESHOLD 5000000ULL

extern const int nr_cpu_ids __ksym;

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__uint(max_entries, 1);
	__type(key, u32);
	__type(value, u64);
} running_mm SEC(".maps");

const volatile u64 vruntime_lag_threshold = VRUNTIME_THRESHOLD;

static __always_inline int get_llc_domain_start(int cpu)
{
	return cpu & ~3;
}

s32 BPF_STRUCT_OPS(cosched_select_cpu,
		   struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
	u64 p_mm = (u64)p->mm;
	u32 key = 0;
	int cpu, llc_start;
	bool found = false;

	llc_start = get_llc_domain_start(prev_cpu);

	bpf_for(cpu, llc_start, llc_start + 4) {
		if (cpu >= nr_cpu_ids)
			break;
		if (scx_bpf_test_and_clear_cpu_idle(cpu))
			return cpu;
	}

	bpf_for(cpu, llc_start, llc_start + 4) {
		u64 *mm_ptr;
		if (cpu >= nr_cpu_ids)
			break;
		mm_ptr = bpf_map_lookup_percpu_elem(&running_mm, &key, cpu);
		if (mm_ptr && *mm_ptr == p_mm && p_mm != 0)
			return cpu;
	}

	cpu = scx_bpf_pick_idle_cpu(p->cpus_ptr, 0);
	if (cpu >= 0)
		return cpu;

	return scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &found);
}

void BPF_STRUCT_OPS(cosched_enqueue, struct task_struct *p, u64 enq_flags)
{
	scx_bpf_dsq_insert(p, SCX_DSQ_GLOBAL, SCX_SLICE_DFL, enq_flags);
}

void BPF_STRUCT_OPS(cosched_running, struct task_struct *p)
{
	u32 key = 0;
	u64 mm = (u64)p->mm;
	bpf_map_update_elem(&running_mm, &key, &mm, BPF_ANY);
}

void BPF_STRUCT_OPS(cosched_stopping, struct task_struct *p, bool runnable)
{
	u32 key = 0;
	u64 zero = 0;
	bpf_map_update_elem(&running_mm, &key, &zero, BPF_ANY);
}

s32 BPF_STRUCT_OPS_SLEEPABLE(cosched_init)
{
	return 0;
}

void BPF_STRUCT_OPS(cosched_exit, struct scx_exit_info *ei)
{
	/* exit info logging omitted */
}

SCX_OPS_DEFINE(cosched_ops,
	.select_cpu	= (void *)cosched_select_cpu,
	.enqueue	= (void *)cosched_enqueue,
	.running	= (void *)cosched_running,
	.stopping	= (void *)cosched_stopping,
	.init		= (void *)cosched_init,
	.exit		= (void *)cosched_exit,
	.name		= "cosched"
);
