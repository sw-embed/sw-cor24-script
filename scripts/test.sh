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

    if echo "$OUTPUT" | grep -qF -- "$expected"; then
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

# Bare words (use unknown command to trigger debug token dump)
run_test "bare words" "_toktest hello world\n\x04" \
    "[WORD|_toktest] [WORD|hello] [WORD|world]"

# Quoted string (unknown command to see tokens)
run_test "quoted string" '_toktest x "hello world"\n\x04' \
    "[WORD|_toktest] [WORD|x] [QUOTED|hello world]"

# Brace block
run_test "brace block" "_toktest cond { echo yes }\n\x04" \
    "[WORD|_toktest] [WORD|cond] [BLOCK|{ echo yes }]"

# Comment stripping
run_test "comment stripping" "_toktest test # comment\n\x04" \
    "[WORD|_toktest] [WORD|test]"

# Comment-only line (no token output, just prompt)
run_test "comment-only line" "# just a comment\n_toktest ok\n\x04" \
    "[WORD|_toktest] [WORD|ok]"

# Multiple spaces
run_test "multiple spaces" "_toktest    hello\n\x04" \
    "[WORD|_toktest] [WORD|hello]"

# Nested braces
run_test "nested braces" "_toktest cond { if inner { echo yes } }\n\x04" \
    "[WORD|_toktest] [WORD|cond] [BLOCK|{ if inner { echo yes } }]"

echo ""
echo "=== Testing sws value types ==="

# Value type test via _valtest command
VALOUT=$("$COR24_RUN" --run "$ASM" -u "_valtest\n\x04" -t 10 2>&1)

check_val() {
    local desc="$1"
    local expected="$2"
    if echo "$VALOUT" | grep -qF -- "$expected"; then
        echo "PASS: $desc"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $desc"
        echo "  Expected: $expected"
        FAIL=$((FAIL + 1))
    fi
}

check_val "integer value"        "int:42"
check_val "zero value"           "zero:0"
check_val "negative value"       "neg:-7"
check_val "string value"         "str:hello"
check_val "empty string"         "empty:"
check_val "record display"       "rec:{status: 0, output: ok}"
check_val "equality true"        "eq:1"
check_val "equality false"       "ne:0"
check_val "truthy int"           "truthy42:1"
check_val "truthy zero"          "truthy0:0"
check_val "truthy string"        "truthyS:1"
check_val "truthy empty"         "truthyE:0"
check_val "truthy record"        "truthyR:1"
check_val "record field get"     "field:0"
check_val "record field has"     "has:1"
check_val "record field miss"    "miss:0"
check_val "total allocations"    "vals:11"

echo ""
echo "=== Testing sws variables ==="

# set and echo
run_test "set/echo int" "set x 42\necho \$x\n\x04" "42"
run_test "set/echo string" "set name hello\necho \$name\n\x04" "hello"
run_test "set/echo negative" "set n -7\necho \$n\n\x04" "-7"
run_test "set/echo quoted" 'set msg "hello world"\necho $msg\n\x04' "hello world"

# Variable overwrite
run_test "var overwrite" "set x 1\nset x 2\necho \$x\n\x04" "2"

# exists? for defined and undefined
run_test "exists? defined" "set x 1\nexists? \$x\n\x04" "1"
run_test "exists? undefined" "exists? \$missing\n\x04" "0"

# Undefined variable error
run_test "undef var error" "echo \$nope\n\x04" "error: undefined variable: nope"

# echo multiple args
run_test "echo multi" "set a hello\nset b world\necho \$a \$b\n\x04" "hello world"

echo ""
echo "=== Testing sws command dispatch ==="

# Unknown command error
run_test "unknown command" "frobnicate\n\x04" "error: unknown command: frobnicate"

# exit command
run_test "exit halts" "echo before\nexit\necho after\n\x04" "before"
# Verify "after" does NOT appear (exit should stop execution)
OUTPUT=$("$COR24_RUN" --run "$ASM" -u "echo before\nexit\necho after\n\x04" -t 5 2>&1)
if echo "$OUTPUT" | grep -qF "after"; then
    echo "FAIL: exit stops execution"
    echo "  Got 'after' in output — exit did not halt"
    FAIL=$((FAIL + 1))
else
    echo "PASS: exit stops execution"
    PASS=$((PASS + 1))
fi

# set error with missing args
run_test "set missing args" "set x\n\x04" "error: set: expected 2 args, got 1"

# Prompt shows sws>
run_test "prompt" "\x04" "sws>"

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
