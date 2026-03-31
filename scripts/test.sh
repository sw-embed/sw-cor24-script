#!/usr/bin/env bash
# test.sh -- Build and verify sws prints its banner
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build"
ASM="$BUILD_DIR/sws.s"
COR24_RUN="cor24-run"

# Build first
./scripts/build.sh build

echo "=== Testing sws banner ==="

OUTPUT=$("$COR24_RUN" --run "$ASM" 2>&1)

if echo "$OUTPUT" | grep -q "sws 0.1"; then
    echo "PASS: banner found"
    echo "  Output: $OUTPUT"
else
    echo "FAIL: expected 'sws 0.1' in output"
    echo "  Got: $OUTPUT"
    exit 1
fi
