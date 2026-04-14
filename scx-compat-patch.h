#ifndef __SCX_COMPAT_PATCH_H
#define __SCX_COMPAT_PATCH_H

/* Patch for kernel backward compatibility with compat.bpf.h
 * Overrides problematic inline functions that instantiate undefined struct types */

/* v6.13+: scx_bpf_select_cpu_and() - fallback to compat version for older kernels */
static inline s32
__scx_bpf_select_cpu_and_fallback(struct task_struct *p, s32 prev_cpu, u64 wake_flags,
		                   const struct cpumask *cpus_allowed, u64 flags)
{
	if (bpf_ksym_exists(scx_bpf_select_cpu_and___compat)) {
		return scx_bpf_select_cpu_and___compat(p, prev_cpu, wake_flags,
						       cpus_allowed, flags);
	}
	return -1;
}

/* v6.13+: scx_bpf_dsq_insert_vtime() - fallback to compat version for older kernels */
static inline bool
__scx_bpf_dsq_insert_vtime_fallback(struct task_struct *p, u64 dsq_id, u64 slice, u64 vtime,
			            u64 enq_flags)
{
	if (bpf_ksym_exists(scx_bpf_dsq_insert_vtime___compat)) {
		scx_bpf_dsq_insert_vtime___compat(p, dsq_id, slice, vtime, enq_flags);
		return true;
	}
	return false;
}

#endif /* __SCX_COMPAT_PATCH_H */
