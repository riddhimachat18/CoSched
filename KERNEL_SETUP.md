# CoSched Kernel Setup Guide

## Issue
When running cosched, you get the error:
```
libbpf: Failed to bump RLIMIT_MEMLOCK (err = -1)
libbpf: Error in bpf_object__probe_loading(): Operation not permitted(1)
Failed to load BPF object: -1
```

## Requirements
- Kernel 6.12+ with `CONFIG_SCHED_CLASS_EXT=y` enabled ✓ (Your kernel has this)
- libbpf development headers
- BPF tools (clang, bpftool)
- Elevated privileges (CAP_BPF, CAP_PERFMON, or root)

## Solution

### 1. Increase RLIMIT_MEMLOCK
Add to `/etc/security/limits.conf`:
```bash
sudo bash -c 'cat >> /etc/security/limits.conf' << 'EOF'
# For BPF/libbpf operations (required for sched_ext)
@sudo   memlock unlimited
*       memlock unlimited
EOF
```

Then logout and login again for changes to take effect.

### 2. Verify kernel support
```bash
# Check kernel version (should be 6.12+)
uname -r

# Verify sched_ext is enabled
cat /proc/config.gz | gunzip | grep CONFIG_SCHED_CLASS_EXT
# Output should be: CONFIG_SCHED_CLASS_EXT=y
```

### 3. Run with sudo (required)
The cosched binary **must** be run with sudo because it loads BPF programs:
```bash
# Test cosched directly
sudo /home/nilay-kumar/CoSched/cosched

# Should output:
# CoSched running. Press Ctrl+C to stop and restore CFS.
# (Press Ctrl+C to stop)

# Run benchmarks
cd /home/nilay-kumar/CoSched/benchmarks
sudo bash ./run_cosched.sh
```

### 4. If you still get "Operation not permitted"
Try setting capabilities instead of using full sudo:
```bash
# Give cosched BPF and perf capabilities
sudo setcap cap_bpf,cap_perfmon,cap_sys_admin,cap_sys_resource=+ep /home/nilay-kumar/CoSched/cosched

# Then run without sudo
/home/nilay-kumar/CoSched/cosched
```

### 5. Configure passwordless sudo (optional)
To avoid entering password for run_cosched.sh:
```bash
sudo visudo
# Add this line at the end:
# nilay-kumar ALL=(ALL) NOPASSWD: /home/nilay-kumar/CoSched/cosched, /usr/bin/perf, /bin/kill
```

## Troubleshooting

### "Permission denied" accessing /sys/kernel/debug/sched_ext/
This is normal - you need root/sudo access. The script uses `sudo` correctly.

### "Cannot load BPF object" after setting RLIMIT_MEMLOCK
- Logout and login again to apply limits.conf changes
- Or manually increase limit: `ulimit -l unlimited` then run with sudo

### "No such file or directory" for cosched.bpf.o
- Rebuild the binary: `cd /home/nilay-kumar/CoSched && make clean && make all`

## Quick Start
```bash
# 1. Update limits
sudo bash -c 'cat >> /etc/security/limits.conf' << 'EOF'
@sudo   memlock unlimited
*       memlock unlimited
EOF

# 2. Logout and login

# 3. Run benchmarks
cd /home/nilay-kumar/CoSched/benchmarks
sudo bash ./run_cosched.sh
```

## References
- [sched_ext GitHub](https://github.com/sched-ext/scx)
- [sched_ext INSTALL guide](https://github.com/sched-ext/scx/blob/main/INSTALL.md)
- [BPF capabilities](https://man7.org/linux/man-pages/man7/capabilities.7.html)
