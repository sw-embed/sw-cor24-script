#!/usr/bin/env bash
# build.sh -- Build sws interpreter for COR24
#
# Usage:
#   ./scripts/build.sh              Build only (compile to assembly)
#   ./scripts/build.sh run          Build and run on emulator
#   ./scripts/build.sh clean        Remove build artifacts
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build"
SRC="src/sws.c"
ASM="$BUILD_DIR/sws.s"

TC24R="tc24r"
TC24R_INCLUDE="/Users/mike/github/sw-embed/sw-cor24-x-tinyc/include"
COR24_RUN="cor24-run"

BIN="$BUILD_DIR/sws.bin"
LST="$BUILD_DIR/sws.lst"

build() {
    mkdir -p "$BUILD_DIR"
    echo "=== Compiling sws ==="
    "$TC24R" "$SRC" -o "$ASM" -I "$TC24R_INCLUDE"
    echo "  $SRC -> $ASM"
    "$COR24_RUN" --assemble "$ASM" "$BIN" "$LST" 2>&1 | head -1
    echo ""
}

run() {
    build
    echo "=== Running on COR24 emulator ==="
    echo ""
    "$COR24_RUN" --run "$ASM" "$@"
}

clean() {
    rm -rf "$BUILD_DIR"
    echo "Cleaned build artifacts."
}

CMD="${1:-build}"

case "$CMD" in
    build)
        build
        echo "Build OK."
        ;;
    run)
        shift
        run "$@"
        ;;
    clean)
        clean
        ;;
    *)
        echo "Usage: $0 {build|run|clean}"
        exit 1
        ;;
esac
