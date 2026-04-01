#!/usr/bin/env bash
# test-all.sh -- Run all sws test suites and report results
set -euo pipefail
cd "$(dirname "$0")/.."

echo "================================================"
echo "  sws — Full Test Suite"
echo "================================================"
echo ""

SUITES_PASS=0
SUITES_FAIL=0

run_suite() {
    local name="$1"
    local script="$2"
    echo "--- $name ---"
    if bash "$script" 2>&1; then
        SUITES_PASS=$((SUITES_PASS + 1))
    else
        echo ""
        echo "*** SUITE FAILED: $name ***"
        SUITES_FAIL=$((SUITES_FAIL + 1))
    fi
    echo ""
}

run_suite "Unit + Integration Tests" "scripts/test.sh"

echo "================================================"
echo "  $SUITES_PASS suite(s) passed, $SUITES_FAIL suite(s) failed"
echo "================================================"

if [ "$SUITES_FAIL" -gt 0 ]; then
    exit 1
fi
