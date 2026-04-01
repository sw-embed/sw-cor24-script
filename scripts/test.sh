#!/usr/bin/env bash
# test.sh -- Build and verify sws tokenizer
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build"
ASM="$BUILD_DIR/sws.s"
COR24_RUN="cor24-run"

# Build first
./scripts/build.sh build

PASS=0
FAIL=0

run_test() {
    local desc="$1"
    local input="$2"
    local expected="$3"

    OUTPUT=$("$COR24_RUN" --run "$ASM" -u "$input" -t 5 2>&1)

    if echo "$OUTPUT" | grep -qF "$expected"; then
        echo "PASS: $desc"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $desc"
        echo "  Expected: $expected"
        echo "  Got: $OUTPUT"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== Testing sws tokenizer ==="

# Banner test
run_test "banner" "\x04" "sws 0.1"

# Bare words
run_test "bare words" "echo hello world\n\x04" \
    "[WORD|echo] [WORD|hello] [WORD|world]"

# Quoted string
run_test "quoted string" 'set x "hello world"\n\x04' \
    "[WORD|set] [WORD|x] [QUOTED|hello world]"

# Brace block
run_test "brace block" "if cond { echo yes }\n\x04" \
    "[WORD|if] [WORD|cond] [BLOCK|{ echo yes }]"

# Comment stripping
run_test "comment stripping" "echo test # comment\n\x04" \
    "[WORD|echo] [WORD|test]"

# Comment-only line (no token output, just prompt)
run_test "comment-only line" "# just a comment\necho ok\n\x04" \
    "[WORD|echo] [WORD|ok]"

# Multiple spaces
run_test "multiple spaces" "echo    hello\n\x04" \
    "[WORD|echo] [WORD|hello]"

# Nested braces
run_test "nested braces" "if cond { if inner { echo yes } }\n\x04" \
    "[WORD|if] [WORD|cond] [BLOCK|{ if inner { echo yes } }]"

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
