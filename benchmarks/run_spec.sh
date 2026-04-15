#!/usr/bin/env bash
# run_spec.sh — specification tests for CoSched
# Test 1: CoSched loads and attaches
# Test 2: shared_array runs under CoSched
# Test 3: CFS is restored after CoSched exits

set -euo pipefail

COSCHED_DIR="$(cd "$(dirname "$0")/.." && pwd)"
COSCHED_BIN="$COSCHED_DIR/cosched"
SHARED_ARRAY_BIN="$COSCHED_DIR/benchmarks/shared_array"
PASS=0
FAIL=0
LOG_FILE="/tmp/cosched_t1.log"

pass() { echo "[PASS] $1"; ((PASS++)) || true; }
fail() { echo "[FAIL] $1"; ((FAIL++)) || true; }

require_root() {
  if [[ $EUID -ne 0 ]]; then
    echo "ERROR: run_spec.sh must be run as root (sudo)."
    exit 1
  fi
}

require_root

cleanup() {
  if [[ -n "${COSCHED_PID:-}" ]] && kill -0 "$COSCHED_PID" 2>/dev/null; then
    kill -SIGINT "$COSCHED_PID" 2>/dev/null || true
    wait "$COSCHED_PID" 2>/dev/null || true
  fi
  rm -f "$LOG_FILE"
}
trap cleanup EXIT

# ── helpers ──────────────────────────────────────────────────────────────────

current_scheduler() {
  if [[ -f /sys/kernel/sched_ext/root/ops ]]; then
    cat /sys/kernel/sched_ext/root/ops 2>/dev/null || echo "none"
  elif [[ -f /sys/kernel/debug/sched/sched_ext ]]; then
    cat /sys/kernel/debug/sched/sched_ext 2>/dev/null || echo "none"
  else
    echo "none"
  fi
}

wait_for_scheduler() {
  local want="$1" timeout="${2:-5}" elapsed=0
  while [[ $elapsed -lt $timeout ]]; do
    local got
    got=$(current_scheduler)
    if [[ "$got" == *"$want"* ]]; then
      return 0
    fi
    sleep 0.5
    ((elapsed++)) || true
  done
  return 1
}

# ── Test 1: CoSched loads and attaches ───────────────────────────────────────
echo ""
echo "=== Test 1: CoSched loads and attaches ==="

if [[ ! -x "$COSCHED_BIN" ]]; then
  fail "Test 1 — cosched binary not found at $COSCHED_BIN"
else
  "$COSCHED_BIN" > "$LOG_FILE" 2>&1 &
  COSCHED_PID=$!

  ATTACHED=0
  for i in $(seq 1 10); do
    if grep -q "CoSched running" "$LOG_FILE" 2>/dev/null; then
      ATTACHED=1
      break
    fi
    sleep 0.5
  done

  if [[ $ATTACHED -eq 1 ]]; then
    pass "Test 1 — CoSched loaded and attached (PID $COSCHED_PID)"
  else
    fail "Test 1 — CoSched did not attach within 5 s"
    cat "$LOG_FILE"
  fi
fi

# ── Test 2: shared_array runs under CoSched ──────────────────────────────────
echo ""
echo "=== Test 2: shared_array runs under CoSched ==="

if [[ ! -x "$SHARED_ARRAY_BIN" ]]; then
  fail "Test 2 — shared_array binary not found at $SHARED_ARRAY_BIN"
else
  if ! kill -0 "${COSCHED_PID:-0}" 2>/dev/null; then
    fail "Test 2 — CoSched is not running; cannot validate"
  else
    SA_OUT=$("$SHARED_ARRAY_BIN" 4 64 5 2>&1)
    SA_EXIT=$?

    if [[ $SA_EXIT -eq 0 ]] && echo "$SA_OUT" | grep -q "total_ops="; then
      TOTAL_OPS=$(echo "$SA_OUT" | grep -oP 'total_ops=\K[0-9]+')
      pass "Test 2 — shared_array completed under CoSched (total_ops=$TOTAL_OPS)"
    else
      fail "Test 2 — shared_array failed or produced unexpected output"
      echo "  Output: $SA_OUT"
    fi
  fi
fi

# ── Test 3: CFS restored after CoSched exits ─────────────────────────────────
echo ""
echo "=== Test 3: CFS restored after CoSched exits ==="

if kill -0 "${COSCHED_PID:-0}" 2>/dev/null; then
  kill -SIGINT "$COSCHED_PID"
  wait "$COSCHED_PID" 2>/dev/null || true
  sleep 1

  SCHED_NOW=$(current_scheduler)
  if [[ "$SCHED_NOW" == "none" ]] || [[ -z "$SCHED_NOW" ]]; then
    pass "Test 3 — CFS restored (sched_ext scheduler = none)"
  else
    INIT_POLICY=$(chrt -p 1 2>/dev/null | grep -oP 'scheduling policy: \K\S+' || echo "unknown")
    if [[ "$INIT_POLICY" == "SCHED_OTHER" ]] || [[ "$INIT_POLICY" == "SCHED_NORMAL" ]]; then
      pass "Test 3 — CFS restored (init scheduling policy = $INIT_POLICY)"
    else
      fail "Test 3 — scheduler after exit: '$SCHED_NOW', init policy: '$INIT_POLICY'"
    fi
  fi
else
  fail "Test 3 — CoSched PID not available; skipping CFS restore check"
fi

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
[[ $FAIL -eq 0 ]]
