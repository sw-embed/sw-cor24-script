# sw-cor24-script

Native COR24 scripting language interpreter — runs directly on COR24 FPGA hardware.

## Overview

`sws` (Software Wrighter Script) is a small, command-oriented scripting
language and interpreter for COR24 systems. Inspired by Tcl's simplicity,
sws provides shell and text-editor scripting with clean semantics:

- **Everything is a command** — words separated by whitespace
- **`{ }` blocks** for control flow bodies and deferred evaluation
- **`" "` quoted strings** for whitespace-containing values
- **`$var` substitution** with `$var.field` record access
- **`(cmd args...)` command substitution** — parenthesized, not bracketed
- **Comparison via commands** — `eq`, `ne`, `lt`, `gt`, `le`, `ge`
- **Arithmetic via commands** — `+`, `-`, `*`, `/`, `%`
- **Structured `$rc` result** from `run` (opt-in via `pragma run-rc on`)
- **`exists?`** for safe variable/field presence testing
- **No hidden state by default** — explicit values, not magic globals

Written in C, compiled by `tc24r` (the cross C compiler from
`sw-cor24-x-tinyc`), and runs natively on COR24 hardware or the emulator.

## Quick Example

```tcl
# FizzBuzz in sws
set i 1
while {le $i 15} {
    if {eq (% $i 15) 0} {
        echo "FizzBuzz"
    } else {
        if {eq (% $i 3) 0} {
            echo "Fizz"
        } else {
            if {eq (% $i 5) 0} {
                echo "Buzz"
            } else {
                echo $i
            }
        }
    }
    incr i
}
```

## Language Reference

### Value Types

| Type | Description | Examples |
|------|-------------|----------|
| Integer | 24-bit signed | `42`, `-7`, `0` |
| String | Byte sequence | `"hello world"`, `hello` |
| Record | Named fields | `{status: 0, output: ok}` |

### Commands

**I/O:**
- `echo arg...` — print arguments separated by spaces, followed by newline

**Variables:**
- `set name value` — set a variable
- `$var` — substitute variable value
- `$var.field` — access record field
- `exists? $var` — 1 if variable exists, 0 otherwise
- `exists? $var.field` — 1 if record field exists, 0 otherwise
- `incr name [amount]` — increment integer variable (default: +1)

**Comparison** (return 1 or 0):
- `eq a b` — equal (integer or string)
- `ne a b` — not equal
- `lt a b` — less than (integer)
- `gt a b` — greater than
- `le a b` — less than or equal
- `ge a b` — greater than or equal

**Logic** (return 1 or 0):
- `and a b` — logical AND
- `or a b` — logical OR
- `not a` — logical NOT

**String:**
- `concat arg...` — concatenate all arguments (no separators)

**Arithmetic:**
- `+ a b` — addition
- `- a b` — subtraction
- `* a b` — multiplication
- `/ a b` — division (errors on zero)
- `% a b` — modulo (errors on zero)

**Control Flow:**
- `if {condition} {body} [else {else-body}]` — conditional
- `while {condition} {body}` — loop
- `break` — exit innermost loop
- `continue` — skip to next iteration
- `exit [code]` — terminate interpreter

**Command Substitution:**
- `(cmd args...)` — evaluate and substitute result inline
- Nesting: `(+ (+ 1 2) (+ 3 4))` → `10`

**Process Execution:**
- `pragma run-rc on` — enable structured `$rc` record
- `run program [args...]` — execute a child binary
- `$rc.run` — 0=ok, 1=not-found, 2=crash, 3=exec-failure, 4=usage-error
- `$rc.kind` — category string
- `$rc.prog` — program exit code (on success)
- `$rc.err`, `$rc.msg` — error details (on failure)

**Filesystem** (stub — awaiting COR24 OS syscalls):
- `cd path` — change directory
- `pwd` — print working directory
- `ls [path]` — list directory
- `mkdir path` — create directory
- `rm path` — remove file
- `mv src dst` — rename/move
- `cp src dst` — copy
- `stat path` — file info
- `fexists path` — 1 if file exists, 0 otherwise

**Source and Environment:**
- `source file.sws` — execute a script file
- `env get NAME` — get process environment variable
- `env set NAME VALUE` — set process environment variable
- `env unset NAME` — remove process environment variable

See [docs/language-reference.md](docs/language-reference.md) for the
complete specification, and [docs/usage.md](docs/usage.md) for a
practical guide with runnable examples.

## Naming Convention

| Repo | Role | Written in | Runs on |
|------|------|-----------|---------|
| `sw-cor24-script` | Native sws interpreter | C | COR24 FPGA |

The `sws` command and `.sws` file extension are consistent across all
Software Wrighter Script implementations on different ISAs (e.g.,
`sw-rv32i-script`, `sw-370-script`).

## Build

```bash
# Compile to COR24 assembly
./scripts/build.sh

# Build and run on emulator
./scripts/build.sh run

# Run tests
./scripts/test.sh

# Clean build artifacts
./scripts/build.sh clean
```

Requires `tc24r` (cross C compiler) and `cor24-run` (emulator) on PATH.

## Bootstrapping

```
sw-cor24-x-tinyc (Rust)  compiles  sws.c  →  sws.s
sw-cor24-x-assembler (Rust)  assembles  sws.s  →  sws.bin
sws.bin runs on COR24 FPGA  →  native script interpreter available on-device
```

## Status

v0.1 — feature-complete interpreter with all core language features
implemented. Filesystem commands are stubbed pending COR24 OS syscall
definitions. See [CHANGES.md](CHANGES.md) for details.

## Related Repos

- [sw-cor24-x-tinyc](https://github.com/sw-embed/sw-cor24-x-tinyc) — Rust cross C compiler (compiles this project)
- [sw-cor24-emulator](https://github.com/sw-embed/sw-cor24-emulator) — COR24 emulator + ISA definitions
- [sw-cor24-assembler](https://github.com/sw-embed/sw-cor24-assembler) — Native assembler in C (same build pattern)
- [sw-cor24-macrolisp](https://github.com/sw-embed/sw-cor24-macrolisp) — Macro Lisp interpreter (sibling project)

## License

See [LICENSE](LICENSE).
