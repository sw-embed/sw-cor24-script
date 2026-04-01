# Changes

## v0.2 (unreleased)

### New Features

- `concat` command: concatenate arguments into a single string with no
  separators. Supports integer auto-conversion and command substitution.
  Example: `concat "x=" (+ 1 2)` → `"x=3"`

## v0.1

Initial release of the sws (Software Wrighter Script) interpreter for COR24.

### Language Features

- Tcl-like command-oriented syntax: everything is a command
- Three value types: integer (24-bit), string, record (named fields)
- Variable environment: `set`, `$var`, `$var.field` record access
- `exists?` for safe variable/field presence testing
- Block evaluation with `{ }` braces (deferred evaluation)
- Quoted strings with `" "` and escape sequences (`\n`, `\t`, `\\`)
- Comment stripping (`#` to end of line)
- Command substitution with `(cmd args...)` parentheses, supporting nesting
- Comparison commands: `eq`, `ne`, `lt`, `gt`, `le`, `ge`
- Logic commands: `and`, `or`, `not`
- Arithmetic commands: `+`, `-`, `*`, `/`, `%`
- Control flow: `if`/`else`, `while`, `break`, `continue`
- `incr` for in-place integer variable increment
- `run` command for child process execution (stub)
- `pragma run-rc on` for structured `$rc` result record
- `source` command for executing script files (requires filesystem)
- `env get`/`set`/`unset` for process environment variables
- Filesystem commands: `cd`, `pwd`, `ls`, `mkdir`, `rm`, `mv`, `cp`,
  `stat`, `fexists` (stub implementations, awaiting OS syscalls)
- Reserved keywords: `try`, `catch`, `finally`, `throw`, `return`,
  `proc`, `for`, `foreach`
- Interactive REPL with `sws>` prompt
- Multi-line brace continuation in REPL input

### Implementation

- Single-file C source (`src/sws.c`, ~2100 lines)
- Compiled by tc24r (cross C compiler for COR24)
- No structs, no malloc, no string library — all static allocation
- Parallel arrays for all data structures
- Function pointer dispatch table for command registration
- Block stack for nested evaluation (avoids deep recursion)
- Comprehensive test suite (100+ test cases)

### Known Limitations

- Filesystem commands are stubs — no COR24 OS syscalls yet
- `run` always returns not-found (no process execution yet)
- `source` requires filesystem read support
- Maximum 64 variables, 64 values, 48 commands
- Maximum 32 tokens per line, 512-byte line length
- No user-defined procedures (`proc` is reserved for future use)
- No `for`/`foreach` loops (reserved for future use)
- No error handling (`try`/`catch` reserved for future use)
