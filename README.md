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
- **Structured `$rc` result** from `run` (opt-in via `pragma run-rc on`)
- **`exists?`** for safe variable/field presence testing
- **No hidden state by default** — explicit values, not magic globals

Written in C, compiled by `tc24r` (the cross C compiler from
`sw-cor24-x-tinyc`), and runs natively on COR24 hardware or the emulator.

## Naming Convention

| Repo | Role | Written in | Runs on |
|------|------|-----------|---------|
| `sw-cor24-script` | Native sws interpreter | C | COR24 FPGA |

The `sws` command and `.sws` file extension are consistent across all
Software Wrighter Script implementations on different ISAs (e.g.,
`sw-rv32i-script`, `sw-370-script`).

## Bootstrapping

```
sw-cor24-x-tinyc (Rust)  compiles  sws.c  →  sws.s
sw-cor24-x-assembler (Rust)  assembles  sws.s  →  sws.bin
sws.bin runs on COR24 FPGA  →  native script interpreter available on-device
```

## Status

In development. See `.agentrail/saga.toml` for implementation progress.

## Related Repos

- [sw-cor24-x-tinyc](https://github.com/sw-embed/sw-cor24-x-tinyc) — Rust cross C compiler (compiles this project)
- [sw-cor24-emulator](https://github.com/sw-embed/sw-cor24-emulator) — COR24 emulator + ISA definitions
- [sw-cor24-assembler](https://github.com/sw-embed/sw-cor24-assembler) — Native assembler in C (same build pattern)
- [sw-cor24-macrolisp](https://github.com/sw-embed/sw-cor24-macrolisp) — Macro Lisp interpreter (sibling project)

## License

See [LICENSE](LICENSE).
