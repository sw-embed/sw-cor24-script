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
echo "=== Testing sws pragma and run ==="

# pragma run-rc on enables $rc
run_test "pragma run-rc on" "pragma run-rc on\nrun something\necho \$rc.run\n\x04" "1"

# $rc.kind after not-found run
run_test "rc.kind not-found" "pragma run-rc on\nrun something\necho \$rc.kind\n\x04" "not-found"

# $rc.msg after not-found run
run_test "rc.msg not-found" "pragma run-rc on\nrun something\necho \$rc.msg\n\x04" "binary not found"

# $rc.err after not-found run
run_test "rc.err not-found" "pragma run-rc on\nrun something\necho \$rc.err\n\x04" "1"

# exists? $rc.err after not-found
run_test "exists rc.err" "pragma run-rc on\nrun something\nexists? \$rc.err\n\x04" "1"

# exists? $rc.prog after not-found (should be 0 — no prog field)
run_test "no rc.prog on not-found" "pragma run-rc on\nrun something\nexists? \$rc.prog\n\x04" "0"

# $rc without pragma → error
run_test "rc without pragma" "echo \$rc\n\x04" "error: undefined variable: rc"

# $rc with pragma but before run → error
run_test "rc before run" "pragma run-rc on\necho \$rc\n\x04" "error: undefined variable: rc"

# pragma with wrong args → error
run_test "pragma bad args" "pragma run-rc\n\x04" "error: pragma: expected 2 args"

# pragma run-rc off → error
run_test "pragma run-rc off" "pragma run-rc off\n\x04" "error: unknown pragma: run-rc off"

# run without args → usage error
run_test "run no args" "pragma run-rc on\nrun\n\x04" "error: run: expected program name"

echo ""
echo "=== Testing sws filesystem commands ==="

# pwd — prints current directory (initialized to /)
run_test "pwd initial" "pwd\n\x04" "/"

# cd and pwd
run_test "cd then pwd" "cd /tmp\npwd\n\x04" "/tmp"

# cd absolute path
run_test "cd absolute" "cd /usr/bin\npwd\n\x04" "/usr/bin"

# cd relative path (appended to cwd)
run_test "cd relative" "cd /home\ncd user\npwd\n\x04" "/home/user"

# cd missing arg → error
run_test "cd no args" "cd\n\x04" "error: cd: expected path argument"

# ls stub → error
run_test "ls stub" "ls\n\x04" "error: ls: filesystem not available"

# mkdir stub → error
run_test "mkdir stub" "mkdir /tmp/test\n\x04" "error: mkdir: filesystem not available"

# mkdir no args → error
run_test "mkdir no args" "mkdir\n\x04" "error: mkdir: expected path argument"

# rm stub → error
run_test "rm stub" "rm /tmp/file\n\x04" "error: rm: filesystem not available"

# rm no args → error
run_test "rm no args" "rm\n\x04" "error: rm: expected path argument"

# mv stub → error
run_test "mv stub" "mv /a /b\n\x04" "error: mv: filesystem not available"

# mv missing args → error
run_test "mv no args" "mv\n\x04" "error: mv: expected 2 args (src dst)"

# cp stub → error
run_test "cp stub" "cp /a /b\n\x04" "error: cp: filesystem not available"

# cp missing args → error
run_test "cp no args" "cp\n\x04" "error: cp: expected 2 args (src dst)"

# stat stub → error
run_test "stat stub" "stat /tmp\n\x04" "error: stat: filesystem not available"

# stat no args → error
run_test "stat no args" "stat\n\x04" "error: stat: expected path argument"

# fexists stub → always 0
run_test "fexists stub" "fexists /tmp\n\x04" "0"

# fexists no args → error
run_test "fexists no args" "fexists\n\x04" "error: fexists: expected path argument"

echo ""
echo "=== Testing sws source command ==="

# source with no args → error
run_test "source no args" "source\n\x04" "error: source: expected filename"

# source nonexistent file → error (fs stub returns -1)
run_test "source not found" "source lib.sws\n\x04" "error: source: cannot read:"

echo ""
echo "=== Testing sws env command ==="

# env set and get
run_test "env set/get" "env set PATH /bin\nenv get PATH\n\x04" "/bin"

# env get undefined → error
run_test "env get undef" "env get MISSING\n\x04" "error: env: undefined: MISSING"

# env set overwrite
run_test "env overwrite" "env set X old\nenv set X new\nenv get X\n\x04" "new"

# env unset
run_test "env unset" "env set Y val\nenv unset Y\nenv get Y\n\x04" "error: env: undefined: Y"

# env bad subcommand → error
run_test "env bad subcmd" "env foo bar\n\x04" "error: env: unknown subcommand: foo"

# env missing args → error
run_test "env no args" "env\n\x04" "error: env: expected subcommand and args"

# env set missing value → error
run_test "env set no val" "env set X\n\x04" "error: env set: expected NAME VALUE"

echo ""
echo "=== Testing sws arithmetic commands ==="

# Basic arithmetic
run_test "add" "echo (+ 1 2)\n\x04" "3"
run_test "subtract" "echo (- 10 3)\n\x04" "7"
run_test "multiply" "echo (* 4 5)\n\x04" "20"
run_test "divide" "echo (/ 15 3)\n\x04" "5"
run_test "modulo" "echo (% 17 5)\n\x04" "2"

# Negative results
run_test "sub negative" "echo (- 3 10)\n\x04" "-7"

# Division by zero
run_test "div by zero" "echo (/ 1 0)\n\x04" "error: /: division by zero"
run_test "mod by zero" "echo (% 1 0)\n\x04" "error: %: division by zero"

echo ""
echo "=== Testing sws command substitution ==="

# Basic command substitution
run_test "cmdsub set" "set x (+ 1 2)\necho \$x\n\x04" "3"
run_test "cmdsub echo" "echo (+ 10 20)\n\x04" "30"

# Nested command substitution
run_test "cmdsub nested" "echo (+ (+ 1 2) (+ 3 4))\n\x04" "10"
run_test "cmdsub deep nest" "echo (+ (+ (+ 1 1) 2) 3)\n\x04" "7"

# Command substitution with variables
run_test "cmdsub with vars" "set a 5\necho (+ \$a 10)\n\x04" "15"

# Command substitution in if condition block
run_test "cmdsub in if" "if {eq (+ 1 1) 2} {echo math works}\n\x04" "math works"

# incr via command substitution
run_test "cmdsub incr loop" "set i 0\nincr i (+ 3 2)\necho \$i\n\x04" "5"

# Unmatched paren
run_test "unmatched paren" "echo (+ 1 2\n\x04" "error: unmatched ("

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
