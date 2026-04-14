CLANG ?= clang
BPFTOOL ?= bpftool
CC ?= gcc
# Suppress errors from compat.bpf.h struct instantiation on older kernels
BPF_FLAGS = -g -O2 -target bpf -I$(HOME)/scx/scheds/include -I$(CURDIR) -fno-strict-aliasing -Wno-error
CFLAGS = -g -O2 -I$(HOME)/scx/scheds/include -I$(CURDIR)

all: vmlinux.h cosched

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

cosched.bpf.o: cosched.bpf.c vmlinux.h cosched.h
	$(CLANG) $(BPF_FLAGS) -c cosched.bpf.c -o cosched.bpf.o

cosched.skel.h: cosched.bpf.o
	$(BPFTOOL) gen skeleton cosched.bpf.o name cosched_bpf > cosched.skel.h

cosched: cosched.c cosched.skel.h
	$(CC) $(CFLAGS) cosched.c -o cosched -lbpf -lelf -lz

clean:
	rm -f cosched cosched.bpf.o cosched.skel.h vmlinux.h
