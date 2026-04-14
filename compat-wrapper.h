#ifndef __SCX_COMPAT_WRAPPER_H
#define __SCX_COMPAT_WRAPPER_H

/* Wrapper to work around compat.bpf.h issues on older kernels
 * This file must be included INSTEAD of compat.bpf.h */

/* Only include the safe parts of compat.bpf.h */
#include <scx/compat.bpf.h>

/* Redefine problematic functions to avoid struct instantiation errors */

#undef scx_bpf_select_cpu_and
#undef scx_bpf_dsq_insert_vtime

/* Fallback implementation of scx_bpf_select_cpu_and for older kernels */
static inline s32
scx_bpf_select_cpu_and(struct task_struct *p, s32 prev_cpu, u64 wake_flags,
		       const struct cpumask *cpus_allowed, u64 flags)
{
	/* Only use the compat version for older kernels */
	return scx_bpf_select_cpu_and___compat(p, prev_cpu, wake_flags,
					       cpus_allowed, flags);
}

/* Fallback implementation of scx_bpf_dsq_insert_vtime for older kernels */
static inline bool
scx_bpf_dsq_insert_vtime(struct task_struct *p, u64 dsq_id, u64 slice, u64 vtime,
			 u64 enq_flags)
{
	if (bpf_ksym_exists(scx_bpf_dsq_insert_vtime___compat)) {
		scx_bpf_dsq_insert_vtime___compat(p, dsq_id, slice, vtime, enq_flags);
		return true;
	}
	return false;
}

#endif /* __SCX_COMPAT_WRAPPER_H */
