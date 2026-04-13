# Makefile for CoSched
CLANG ?= clang
CC ?= gcc
BPFTOOL ?= bpftool
INCLUDES = -I. -I/usr/include/bpf
CFLAGS = -g -O2 -Wall
BPF_FLAGS = -g -O2 -target bpf $(INCLUDES)
all: vmlinux.h cosched
vmlinux.h:
$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c &gt; vmlinux.h
cosched.bpf.o: cosched.bpf.c vmlinux.h cosched.h
$(CLANG) $(BPF_FLAGS) -c cosched.bpf.c -o cosched.bpf.o
cosched.skel.h: cosched.bpf.o
$(BPFTOOL) gen skeleton cosched.bpf.o name cosched_bpf &gt; cosched.skel.h
cosched: cosched.c cosched.skel.h
$(CC) $(CFLAGS) cosched.c -o cosched -lbpf -lelf -lz
clean:
rm -f cosched cosched.bpf.o cosched.skel.h vmlinux.h
.PHONY: all clean