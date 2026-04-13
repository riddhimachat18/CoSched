// cosched.bpf.c

// Cache-collocated scheduler using sched_ext
// Build: make (see Makefile)
#include &lt;scx/common.bpf.h&gt;
#include &quot;vmlinux.h&quot;
#include &quot;cosched.h&quot;
// ── Configuration (tunable via BPF map) ──────────────────────────
#define COSCHED_VERSION 1
#define MAX_CPUS 512
#define VRUNTIME_THRESHOLD 5000000ULL // 5ms in nanoseconds
// ── Global state ─────────────────────────────────────────────────
// Per-CPU slot: stores the mm pointer of the task currently running
struct {
__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
__uint(max_entries, 1);
__type(key, u32);
__type(value, u64); // stores (u64)current-&gt;mm
} running_mm SEC(&quot;.maps&quot;);
// Tunable threshold (writable from user-space via bpftool)
const volatile u64 vruntime_lag_threshold = VRUNTIME_THRESHOLD;
// ── Helper: get LLC domain for a given CPU ────────────────────────
// Returns the first CPU in the LLC domain of &#39;cpu&#39;.
// Used to check co-location eligibility.
static __always_inline int get_llc_domain_start(int cpu)
{
// Walk sched_domain hierarchy to find SD_SHARE_PKG_RESOURCES level
// NOTE: actual domain walk requires kernel helpers — simplified here.
// In practice, precompute LLC domain masks at load time in user-space
// and pass them in via a BPF array map.
return cpu &amp; ~3; // simplified: assume groups of 4 (replace with map
lookup)
}
// ── ops.select_cpu ────────────────────────────────────────────────
// Called when task &#39;p&#39; becomes runnable.
// Returns the CPU we want to run &#39;p&#39; on.
s32 BPF_STRUCT_OPS(cosched_select_cpu,
struct task_struct *p,
s32 prev_cpu,
u64 wake_flags)
{
u64 p_mm = (u64)p-&gt;mm;
u64 p_vrt = p-&gt;se.vruntime;
u64 min_vrt = scx_bpf_dsq_nr_queued(SCX_DSQ_GLOBAL);
u32 key = 0;
int cpu, llc_start;
// ── Step 1: fairness override ──────────────────────────────────
// If this task is lagging too far behind, skip locality and
// just pick the least loaded CPU.
if (p_vrt + vruntime_lag_threshold &lt; min_vrt) {

return scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, NULL);
}
// ── Step 2: prefer idle CPU in same LLC domain ─────────────────
llc_start = get_llc_domain_start(prev_cpu);
bpf_for(cpu, llc_start, llc_start + 4) {
if (cpu &gt;= nr_cpu_ids) break;
if (scx_bpf_test_and_clear_cpu_idle(cpu)) {
return cpu; // idle in same LLC: best placement
}
}
// ── Step 3: co-locate with same-mm task ────────────────────────
bpf_for(cpu, llc_start, llc_start + 4) {
u64 *mm_ptr;
if (cpu &gt;= nr_cpu_ids) break;
mm_ptr = bpf_map_lookup_percpu_elem(&amp;running_mm, &amp;key, cpu);
if (mm_ptr &amp;&amp; *mm_ptr == p_mm &amp;&amp; p_mm != 0) {
return cpu; // same mm running on this CPU: co-locate
}
}
// ── Step 4: any idle CPU ───────────────────────────────────────
cpu = scx_bpf_pick_idle_cpu(p-&gt;cpus_ptr, 0);
if (cpu &gt;= 0) return cpu;
// ── Step 5: full CFS fallback ──────────────────────────────────
return scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, NULL);
}
// ── ops.running ───────────────────────────────────────────────────
// Called when task p starts running on a CPU.
// Record its mm pointer so other tasks can detect co-location.
void BPF_STRUCT_OPS(cosched_running, struct task_struct *p)
{
u32 key = 0;
u64 mm = (u64)p-&gt;mm;
bpf_map_update_elem(&amp;running_mm, &amp;key, &amp;mm, BPF_ANY);
}
// ── ops.stopping ──────────────────────────────────────────────────
// Called when task p stops running. Clear the mm slot.
void BPF_STRUCT_OPS(cosched_stopping,
struct task_struct *p, bool runnable)
{
u32 key = 0;
u64 zero = 0;
bpf_map_update_elem(&amp;running_mm, &amp;key, &amp;zero, BPF_ANY);
}
// ── ops.enqueue ───────────────────────────────────────────────────
// Called to place a task into a dispatch queue.
// We use the global DSQ with CFS-compatible vruntime ordering.
void BPF_STRUCT_OPS(cosched_enqueue,
struct task_struct *p, u64 enq_flags)

{
scx_bpf_dispatch(p, SCX_DSQ_GLOBAL, SCX_SLICE_DFL, enq_flags);
}
// ── ops initialisation ────────────────────────────────────────────
s32 BPF_STRUCT_OPS_SLEEPABLE(cosched_init)
{
return 0;
}
void BPF_STRUCT_OPS(cosched_exit, struct scx_exit_info *ei)
{
UEI_RECORD(uei, ei);
}
SCX_OPS_DEFINE(cosched_ops,
.select_cpu = (void *)cosched_select_cpu,
.enqueue = (void *)cosched_enqueue,
.running = (void *)cosched_running,
.stopping = (void *)cosched_stopping,
.init = (void *)cosched_init,
.exit = (void *)cosched_exit,
.name = &quot;cosched&quot;
);