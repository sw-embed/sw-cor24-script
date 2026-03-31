# sw-cor24-script — Claude Instructions

## Project Overview

Native COR24 scripting language interpreter (`sws` — Software Wrighter
Script) written in C. A Tcl-like command language for shell and editor
scripting on COR24 FPGA hardware.

The C source is compiled by `tc24r` (the cross C compiler from
`sw-cor24-x-tinyc`) and assembled by `as24` (the cross-assembler from
`sw-cor24-x-assembler`). The resulting binary runs on COR24 hardware
or the emulator (`sw-cor24-emulator`).

## CRITICAL: AgentRail Session Protocol (MUST follow exactly)

### 1. START (do this FIRST, before anything else)
```bash
agentrail next
```
Read the output carefully. It contains your current step, prompt,
plan context, and any relevant skills/trajectories.

### 2. BEGIN (immediately after reading the next output)
```bash
agentrail begin
```

### 3. WORK (do what the step prompt says)
Do NOT ask "want me to proceed?". The step prompt IS your instruction.
Execute it directly.

### 4. COMMIT (after the work is done)
Commit your code changes with git. Use `/mw-cp` for the checkpoint
process (pre-commit checks, docs, detailed commit, push).

### 5. COMPLETE (LAST thing, after committing)
```bash
agentrail complete --summary "what you accomplished" \
  --reward 1 \
  --actions "tools and approach used"
```
- If the step failed: `--reward -1 --failure-mode "what went wrong"`
- If the saga is finished: add `--done`

### 6. STOP (after complete, DO NOT continue working)
Do NOT make further code changes after running `agentrail complete`.
Any changes after complete are untracked and invisible to the next
session. Future work belongs in the NEXT step, not this one.

## Key Rules

- **Do NOT skip steps** — the next session depends on accurate tracking
- **Do NOT ask for permission** — the step prompt is the instruction
- **Do NOT continue working** after `agentrail complete`
- **Commit before complete** — always commit first, then record completion

## Useful Commands

```bash
agentrail status          # Current saga state
agentrail history         # All completed steps
agentrail plan            # View the plan
agentrail next            # Current step + context
```

## Build / Test

```bash
# Compile with tc24r (cross C compiler)
tc24r src/sws.c -o sws.s -I include/

# Assemble with as24 (cross-assembler)
# ... produces sws.bin

# Run on emulator
cor24-run sws.bin

# Test: run a .sws script and verify output
cor24-run sws.bin < test.sws
```

## Architecture

- Tcl-like command language: everything is a command
- Tokenizer → word list → command dispatch (no AST initially)
- Value types: integer, string, record (for $rc)
- Variables via set/$var with $var.field record field access
- Command substitution via (cmd args...) parentheses
- Comparison commands: eq, ne, lt, gt, le, ge
- Structured run result via $rc record (pragma run-rc on)
- Result<Value, ScriptError> error model throughout
- Input: .sws scripts via UART or memory buffer
- Output: UART character I/O

## tc24r C Subset Constraints

- No structs — use parallel arrays
- No malloc/free — static allocation only
- No string library — hand-roll str_eq, str_len, etc.
- No varargs/printf — use direct UART output
- Single translation unit — one .c file (or #include chaining)
- 24-bit int, 8-bit char, pointers

## Cross-Repo Context

All COR24 repos live under `~/github/sw-embed/` as siblings:
- `sw-cor24-emulator` — emulator + ISA (foundation)
- `sw-cor24-x-assembler` — cross-assembler in Rust (reference implementation)
- `sw-cor24-x-tinyc` — cross C compiler in Rust (builds this project)
- `sw-cor24-assembler` — native assembler in C (same build pattern)
- `sw-cor24-macrolisp` — Macro Lisp interpreter in C (similar project)
