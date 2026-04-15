#!/usr/bin/env python3
"""
plot_results.py -- generates all figures for the paper
Usage:  python3 plot_results.py
Output: figures saved to ~/cosched_results/figures/
"""

import csv, os, sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
BASE_CSV    = os.path.join(SCRIPT_DIR, 'baseline_results.csv')
COSCHED_CSV = os.path.join(SCRIPT_DIR, 'cosched_results.csv')
FIG_DIR     = os.path.expanduser('~/cosched_results/figures')
os.makedirs(FIG_DIR, exist_ok=True)


def load(path):
    data = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = int(row['threads'])
            if t not in data:
                data[t] = []
            data[t].append(float(row['llc_miss_rate_pct']))
    return data


def load_ipc(path):
    data = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = int(row['threads'])
            if t not in data:
                data[t] = []
            data[t].append(float(row['ipc']))
    return data


def load_wall(path):
    data = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = int(row['threads'])
            if t not in data:
                data[t] = []
            data[t].append(float(row['wall_sec']))
    return data


threads_list = [2, 4, 8, 16]

for csv_path in [BASE_CSV, COSCHED_CSV]:
    if not os.path.exists(csv_path):
        print(f'Missing: {csv_path}')
        sys.exit(1)

base_miss = load(BASE_CSV)
cs_miss   = load(COSCHED_CSV)
base_ipc  = load_ipc(BASE_CSV)
cs_ipc    = load_ipc(COSCHED_CSV)
base_wall = load_wall(BASE_CSV)
cs_wall   = load_wall(COSCHED_CSV)

x = np.arange(len(threads_list))
w = 0.35

# --- Figure 1: LLC miss rate ---
fig, ax = plt.subplots(figsize=(8, 5))
bm = [np.mean(base_miss[t]) for t in threads_list]
cm = [np.mean(cs_miss[t])   for t in threads_list]
be = [np.std(base_miss[t])  for t in threads_list]
ce = [np.std(cs_miss[t])    for t in threads_list]
ax.bar(x - w/2, bm, w, yerr=be, label='CFS (baseline)', capsize=4, color='#4A6FA5')
ax.bar(x + w/2, cm, w, yerr=ce, label='CoSched',        capsize=4, color='#27AE60')
ax.set_xlabel('Number of threads')
ax.set_ylabel('LLC miss rate (%)')
ax.set_title('LLC Miss Rate: CFS vs CoSched')
ax.set_xticks(x)
ax.set_xticklabels([str(t) for t in threads_list])
ax.legend()
ax.grid(axis='y', alpha=0.3)
plt.tight_layout()
plt.savefig(f'{FIG_DIR}/fig_llc_miss_rate.png', dpi=150)
print(f'Saved: {FIG_DIR}/fig_llc_miss_rate.png')
plt.close()

# --- Figure 2: IPC ---
fig, ax = plt.subplots(figsize=(8, 5))
bi  = [np.mean(base_ipc[t]) for t in threads_list]
ci  = [np.mean(cs_ipc[t])   for t in threads_list]
bei = [np.std(base_ipc[t])  for t in threads_list]
cei = [np.std(cs_ipc[t])    for t in threads_list]
ax.bar(x - w/2, bi, w, yerr=bei, label='CFS (baseline)', capsize=4, color='#4A6FA5')
ax.bar(x + w/2, ci, w, yerr=cei, label='CoSched',        capsize=4, color='#27AE60')
ax.set_xlabel('Number of threads')
ax.set_ylabel('Instructions per cycle (IPC)')
ax.set_title('IPC: CFS vs CoSched')
ax.set_xticks(x)
ax.set_xticklabels([str(t) for t in threads_list])
ax.legend()
ax.grid(axis='y', alpha=0.3)
plt.tight_layout()
plt.savefig(f'{FIG_DIR}/fig_ipc.png', dpi=150)
print(f'Saved: {FIG_DIR}/fig_ipc.png')
plt.close()

# --- Figure 3: Wall-clock time ---
fig, ax = plt.subplots(figsize=(8, 5))
bw  = [np.mean(base_wall[t]) for t in threads_list]
cw  = [np.mean(cs_wall[t])   for t in threads_list]
bwe = [np.std(base_wall[t])  for t in threads_list]
cwe = [np.std(cs_wall[t])    for t in threads_list]
ax.bar(x - w/2, bw, w, yerr=bwe, label='CFS (baseline)', capsize=4, color='#4A6FA5')
ax.bar(x + w/2, cw, w, yerr=cwe, label='CoSched',        capsize=4, color='#27AE60')
ax.set_xlabel('Number of threads')
ax.set_ylabel('Wall-clock time (seconds)')
ax.set_title('Wall-Clock Time: CFS vs CoSched')
ax.set_xticks(x)
ax.set_xticklabels([str(t) for t in threads_list])
ax.legend()
ax.grid(axis='y', alpha=0.3)
plt.tight_layout()
plt.savefig(f'{FIG_DIR}/fig_walltime.png', dpi=150)
print(f'Saved: {FIG_DIR}/fig_walltime.png')
plt.close()

# --- Figure 4: Scalability (miss rate improvement %) ---
fig, ax = plt.subplots(figsize=(8, 5))
impr = [bm[i] - cm[i] for i in range(len(threads_list))]
ax.plot([str(t) for t in threads_list], impr, 'o-',
        color='#27AE60', linewidth=2, markersize=8)
ax.axhline(0, color='gray', linestyle='--', linewidth=1)
ax.fill_between([str(t) for t in threads_list], impr, 0,
                alpha=0.15, color='#27AE60')
ax.set_xlabel('Number of threads')
ax.set_ylabel('LLC miss rate reduction (percentage points)')
ax.set_title('CoSched Improvement Over CFS (scalability)')
ax.grid(alpha=0.3)
plt.tight_layout()
plt.savefig(f'{FIG_DIR}/fig_scalability.png', dpi=150)
print(f'Saved: {FIG_DIR}/fig_scalability.png')
plt.close()

# --- Summary table ---
print()
print('=' * 70)
print(f'{"Threads":>8} | {"CFS miss%":>10} | {"CS miss%":>10} | {"CFS IPC":>8} | {"CS IPC":>8}')
print('-' * 70)
for i, t in enumerate(threads_list):
    print(f'{t:>8} | {bm[i]:>10.2f} | {cm[i]:>10.2f} | {bi[i]:>8.3f} | {ci[i]:>8.3f}')
print('=' * 70)
