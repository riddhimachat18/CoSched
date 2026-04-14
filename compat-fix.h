#ifndef __COMPAT_FIX_H
#define __COMPAT_FIX_H

/* Workaround for kernel versions that don't have the newer sched_ext structs.
 * This patches the compat.bpf.h functions to avoid struct instantiation
 * when the types don't exist in the kernel BTF.
 */

/* For kernels < 6.13/6.14 that don't have scx_bpf_select_cpu_and_args */
#if 1
static inline s32
scx_bpf_select_cpu_and(struct task_struct *p, s32 prev_cpu, u64 wake_flags,
		       const struct cpumask *cpus_allowed, u64 flags)
{
	/* Fallback to the compat version */
	return scx_bpf_select_cpu_and___compat(p, prev_cpu, wake_flags,
					       cpus_allowed, flags);
}
#endif

/* For kernels < 6.13/6.14 that don't have scx_bpf_dsq_insert_vtime_args */
#if 1
static inline bool
scx_bpf_dsq_insert_vtime(struct task_struct *p, u64 dsq_id, u64 slice, u64 vtime,
			 u64 enq_flags)
{
	/* Fallback to the compat version */
	if (bpf_ksym_exists(scx_bpf_dsq_insert_vtime___compat)) {
		scx_bpf_dsq_insert_vtime___compat(p, dsq_id, slice, vtime,
						  enq_flags);
		return true;
	}
	return false;
}
#endif

#endif /* __COMPAT_FIX_H */
