// cosched.c — loads and attaches the BPF scheduler
#include &lt;stdio.h&gt;
#include &lt;signal.h&gt;
#include &lt;unistd.h&gt;
#include &lt;bpf/libbpf.h&gt;
#include &quot;cosched.skel.h&quot;
static volatile bool exiting = false;
static void handle_sigint(int sig) { exiting = true; }
int main(int argc, char **argv)
{
struct cosched_bpf *skel;
struct bpf_link *link;
int err;
signal(SIGINT, handle_sigint);
signal(SIGTERM, handle_sigint);
// Open and load BPF skeleton
skel = cosched_bpf__open_and_load();
if (!skel) { fprintf(stderr, &quot;Failed to open/load BPF skeleton\n&quot;); return
1; }
// Attach the scheduler
err = cosched_bpf__attach(skel);

if (err) { fprintf(stderr, &quot;Failed to attach: %d\n&quot;, err); goto cleanup; }
printf(&quot;CoSched running. Press Ctrl+C to stop and restore CFS.\n&quot;);
while (!exiting) { sleep(1); }
printf(&quot;\nStopping CoSched, restoring CFS...\n&quot;);
cleanup:
cosched_bpf__destroy(skel);
return err;
}