#!/usr/bin/env bash
# run_tests.sh — ORION CI entry point
# Usage: ./run_tests.sh
# Exit code: 0 = all passed, 1 = failures detected

# Why set -e?
# The script exits immediately if ANY command fails.
# Without it, a failed cmake would let the script keep running
# and try to execute a binary that doesn't exist.
# In CI, silent continuation after failure hides the real problem.
set -e

# Why set -o pipefail?
# set -e doesn't catch failures inside pipes.
# cmd1 | cmd2 — if cmd1 fails, without pipefail the pipe
# returns cmd2's exit code and set -e misses the failure.
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
LOG_FILE="$SCRIPT_DIR/logs/run.json"

echo "========================================"
echo "  ORION CI Pipeline"
echo "  $(date)"
echo "========================================"

# --- Step 1: Build ---
echo ""
echo "[1/3] Building..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
make -j4

# Why -j4? Parallel compilation — use 4 CPU cores simultaneously.
# On a large codebase this cuts build time dramatically.
# In CI this matters because build time = engineer waiting time.

echo "  Build successful."

# --- Step 2: Run tests ---
echo ""
echo "[2/3] Running tests..."
cd "$SCRIPT_DIR"

if [ "$1" == "--chaos" ]; then
    echo "  ⚠️  CHAOS MODE ENABLED"
    ORION_LOG="$SCRIPT_DIR/logs/run.json" ORION_CHAOS=1 ./build/orion_tests || TEST_EXIT=$?
else
    ORION_LOG="$SCRIPT_DIR/logs/run.json" ./build/orion_tests || TEST_EXIT=$?
fi
TEST_EXIT=${TEST_EXIT:-0}

if [ $TEST_EXIT -eq 0 ]; then
    echo "  Tests passed."
else
    echo "  Tests reported failures (exit code: $TEST_EXIT)."
fi

# --- Step 3: Generate report ---
echo ""
echo "[3/3] Generating report..."
python3 reporter/report.py --input "$LOG_FILE"
REPORT_EXIT=$?

# --- Final exit ---
# Exit with failure if either tests OR reporter indicated failure
if [ $TEST_EXIT -ne 0 ] || [ $REPORT_EXIT -ne 0 ]; then
    echo "[CI] PIPELINE FAILED"
    exit 1
fi

echo "[CI] PIPELINE PASSED"
exit 0