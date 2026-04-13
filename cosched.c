// cosched.c — loads and attaches the BPF scheduler
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include "cosched.skel.h"
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
if (!skel) { fprintf(stderr, "Failed to open/load BPF skeleton\n"); return
1; }
// Attach the scheduler
err = cosched_bpf__attach(skel);

if (err) { fprintf(stderr, "Failed to attach: %d\n", err); goto cleanup; }
printf("CoSched running. Press Ctrl+C to stop and restore CFS.\n");
while (!exiting) { sleep(1); }
printf("\nStopping CoSched, restoring CFS...\n");
cleanup:
cosched_bpf__destroy(skel);
return err;
}