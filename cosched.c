// cosched.c — loads and attaches the BPF scheduler
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <bpf/libbpf.h>
static volatile bool exiting = false;
static void handle_sigint(int sig) { exiting = true; }
int main(int argc, char **argv)
{
	struct bpf_object *obj;
	struct bpf_program *prog;
	struct bpf_link *link;
	int err;
	char bpf_path[256];
	char self_path[256];
	char *dir;
	
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	
	// Get the directory of the running binary
	ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
	if (len < 0) {
		fprintf(stderr, "Failed to get binary path\n");
		return 1;
	}
	self_path[len] = '\0';
	dir = dirname(self_path);
	
	// Construct path to cosched.bpf.o
	snprintf(bpf_path, sizeof(bpf_path), "%s/cosched.bpf.o", dir);
	
	// Note: sched_ext will be enabled automatically when the BPF struct_ops
	// is attached below. No need to manually write to switch_all.
	// Open and load BPF object
	obj = bpf_object__open_file(bpf_path, NULL);
	if (!obj) { fprintf(stderr, "Failed to open BPF object\n"); return 1; }
	err = bpf_object__load(obj);
	if (err) { fprintf(stderr, "Failed to load BPF object: %d\n", err); goto cleanup; }
	// Find the struct ops map
	struct bpf_map *map = bpf_object__find_map_by_name(obj, "cosched_ops");
	if (!map) { fprintf(stderr, "Failed to find map 'cosched_ops'\n"); goto cleanup; }
	// Attach the scheduler
	link = bpf_map__attach_struct_ops(map);
	if (!link) { fprintf(stderr, "Failed to attach\n"); goto cleanup; }
	printf("CoSched running. Press Ctrl+C to stop and restore CFS.\n");
	fflush(stdout);
	// Check if link is correctly attached
	fprintf(stderr, "Debug: Link attached successfully\n");
	fflush(stderr);
	while (!exiting) { sleep(1); }
	printf("\nStopping CoSched, restoring CFS...\n");
cleanup:
	if (link) bpf_link__destroy(link);
	bpf_object__close(obj);
	return err;
}