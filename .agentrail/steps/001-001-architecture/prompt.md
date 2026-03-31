Design the sws architecture and set up build/test infrastructure.

1. Create src/sws.c with main(), UART I/O helpers, and memory layout constants.
   - UART read/write character functions (memory-mapped I/O)
   - uart_puts() for string output, uart_putint() for integer output
   - Define static allocation limits:
     - MAX_LINE_LEN = 4096 (input line buffer)
     - MAX_TOKENS = 256 (tokens per line)
     - MAX_TOKEN_LEN = 256 (single token length)
     - MAX_VARS = 256 (variable slots)
     - MAX_VAR_NAME = 64 (variable name length)
     - MAX_VAR_VAL = 256 (variable value length)
     - MAX_NEST = 64 (block nesting depth)
     - MAX_FIELDS = 16 (record fields per variable)

2. Create scripts/build.sh that compiles with tc24r and assembles with as24.

3. Create scripts/test.sh that runs sws on a trivial .sws input and verifies
   output via the emulator.

4. Implement hand-rolled string helpers:
   - str_eq(a, b) — compare two strings for equality
   - str_len(s) — return string length
   - str_copy(dst, src) — copy string
   - str_starts_with(s, prefix) — check prefix
   - str_to_int(s) — parse integer from string
   - int_to_str(buf, n) — format integer to string

5. Verify the skeleton compiles and runs (prints a banner via UART):
   "sws 0.1 — Software Wrighter Script"

Reference architecture: ~/github/sw-embed/sw-cor24-assembler/src/cas24.c
Reference build: ~/github/sw-embed/sw-cor24-assembler/scripts/build.sh
Design doc: docs/research.txt