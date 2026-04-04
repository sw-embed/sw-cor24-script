#!/bin/bash
# editor-demo.sh -- sws launches swye, edits a file, reads back the result
#
# This demo co-loads two COR24 binaries in the emulator:
#   sws.s   at 0x000000  -- the script interpreter
#   swye    at 0x080000  -- the text editor
#   text    at 0x010000  -- file to edit
#   cmds    at 0x0F0000  -- pre-loaded editor keystrokes
#
# sws's `run swye` calls swye._main via function pointer.
# swye reads commands from 0x0F0000 (not UART), edits, quits, returns.
# sws reads the edited buffer from $rc.output.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
YOCTO_ED="$PROJECT_DIR/../sw-cor24-yocto-ed"

# Check prerequisites
if [ ! -f "$YOCTO_ED/build/swye.s" ]; then
    echo "error: swye not built. Run 'just build' in sw-cor24-yocto-ed first."
    exit 1
fi

# Build sws
echo "Building sws..."
tc24r "$PROJECT_DIR/src/sws.c" -o "$PROJECT_DIR/sws.s" -I "$PROJECT_DIR/include/"

# Assemble swye at base 0x080000
echo "Assembling swye at 0x080000..."
cor24-run --assemble "$YOCTO_ED/build/swye.s" /tmp/swye.bin /tmp/swye.lst \
    --base-addr 0x080000 2>&1 | head -1

# Find swye._main address from listing
SWYE_MAIN=$(grep '_main:' /tmp/swye.lst -A 1 | grep -o '^[0-9a-f]*' | head -1)
echo "swye._main at 0x${SWYE_MAIN}"

# Create editor command file: move to "red", delete 3 chars, insert "blue", quit
printf '\x1b10 right\ndel\ndel\ndel\n\x1bblue\x1bquit\n' > /tmp/swye_cmds.bin

TESTFILE="$YOCTO_ED/tests/testfile.txt"
echo ""
echo "=== Input ==="
cat "$TESTFILE"
echo ""
echo "Edit plan: replace 'red' with 'blue'"
echo ""

# Run: sws script calls `run swye`, swye reads from command buffer
SWS_SCRIPT="$(cat "$PROJECT_DIR/docs/examples/editor-test.sws")"

OUTPUT=$(cor24-run --run "$PROJECT_DIR/sws.s" \
    --load-binary /tmp/swye.bin@0x080000 \
    --load-binary "$TESTFILE@0x010000" \
    --load-binary /tmp/swye_cmds.bin@0x0F0000 \
    --patch "0x0FFE00=0x0${SWYE_MAIN}" \
    --speed 0 --stack-kilobytes 8 \
    -u "${SWS_SCRIPT}"$'\x04' \
    -t 30 2>&1)

# Show sws output (skip editor render lines and prompts)
echo "=== sws output ==="
echo "$OUTPUT" | grep -A 9999 '^UART output:' | tail -n +2 | \
    grep -v '^Executed\|^CPU\|^TEXT>\|^MODE>\|^CMD >' | \
    grep -v '^sws>' | grep -v '^[[:space:]]*$' | head -20

# Verify
if echo "$OUTPUT" | grep -q 'The quick blue fox'; then
    echo ""
    echo "PASS: sws launched swye, edited red->blue, read back the result"
else
    echo ""
    echo "FAIL: edit not applied or output not captured"
    exit 1
fi
