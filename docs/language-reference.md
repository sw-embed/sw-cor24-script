# sws Language Reference

Software Wrighter Script (sws) v0.1 — a Tcl-like command language for
COR24 systems.

## Syntax

### Commands

Everything in sws is a command. A line consists of a command name
followed by zero or more arguments, separated by whitespace:

```tcl
echo hello world
set x 42
```

### Comments

`#` begins a comment that extends to end of line:

```tcl
echo hello  # this is a comment
# this entire line is a comment
```

### Quoted Strings

Double quotes group words into a single argument:

```tcl
set msg "hello world"
echo "multi word argument"
```

Escape sequences in quoted strings:
- `\n` — newline
- `\t` — tab
- `\\` — literal backslash
- `\"` — literal double quote

### Blocks

Braces `{ }` create blocks — deferred code that is not evaluated
immediately. Used for control flow bodies and conditions:

```tcl
if {eq $x 1} {
    echo "x is one"
}
```

Blocks can span multiple lines (braces track nesting). Blocks are
not variable-expanded until evaluated.

### Variable Substitution

`$name` substitutes the variable's value:

```tcl
set x 42
echo $x          # prints: 42
```

`$name.field` accesses a field of a record variable:

```tcl
pragma run-rc on
run something
echo $rc.kind    # prints: not-found
```

Referencing an undefined variable is a runtime error.

### Command Substitution

Parentheses `(cmd args...)` evaluate a command and substitute its
result inline:

```tcl
echo (+ 1 2)              # prints: 3
set sum (+ (* 3 4) 5)     # sum = 17
if {eq (% $i 2) 0} {echo even}
```

Command substitution processes innermost parentheses first, supporting
arbitrary nesting:

```tcl
echo (+ (+ 1 2) (+ 3 4))  # prints: 10
```

## Value Types

### Integer

24-bit signed integer. Created from numeric literals:

```tcl
set x 42
set y -7
set z 0
```

**Truthiness:** nonzero is true, zero is false.

### String

Byte sequence. Created from bare words or quoted strings:

```tcl
set name hello
set msg "hello world"
```

**Truthiness:** nonempty is true, empty string is false.

### Record

Named-field collection. Created by certain commands (e.g., `run`
with `pragma run-rc on`). Fields accessed with `$var.field` syntax:

```tcl
pragma run-rc on
run something
echo $rc.run     # 1
echo $rc.kind    # not-found
echo $rc.msg     # binary not found
```

**Truthiness:** records are always true.

## Commands

### I/O

#### echo

```
echo [arg...]
```

Print arguments separated by spaces, followed by a newline. With no
arguments, prints an empty line.

### Variables

#### set

```
set name value
```

Assign `value` to variable `name`. Creates the variable if it doesn't
exist. Value is auto-detected as integer (if all digits with optional
leading minus) or string.

#### exists?

```
exists? $var
exists? $var.field
```

Returns 1 if the variable (or record field) exists, 0 otherwise.
Never produces an error — safe to use on undefined variables. Uses
raw tokens (before variable expansion).

#### incr

```
incr name [amount]
```

Increment an integer variable by `amount` (default 1). The variable
must already exist and hold an integer value.

```tcl
set i 0
incr i       # i = 1
incr i 5     # i = 6
incr i -2    # i = 4
```

### Comparison Commands

All comparison commands return 1 (true) or 0 (false).

#### eq, ne

```
eq a b
ne a b
```

Equal / not-equal. For two integer-like strings, compares numerically.
Otherwise compares as strings.

#### lt, gt, le, ge

```
lt a b
gt a b
le a b
ge a b
```

Less-than, greater-than, less-or-equal, greater-or-equal. Integer
comparison only.

### Logic Commands

All logic commands return 1 (true) or 0 (false).

```
and a b    # true if both truthy
or  a b    # true if either truthy
not a      # true if falsy
```

### Arithmetic Commands

```
+ a b      # addition
- a b      # subtraction
* a b      # multiplication
/ a b      # division (error on zero)
% a b      # modulo (error on zero)
```

All operate on integer values. Division by zero produces a runtime error.

### Control Flow

#### if

```
if {condition} {body}
if {condition} {body} else {else-body}
```

Evaluate the condition block. If the result is truthy, evaluate the
body block. Otherwise evaluate the else-body (if present).

```tcl
if {gt $x 0} {
    echo "positive"
} else {
    echo "non-positive"
}
```

#### while

```
while {condition} {body}
```

Repeatedly evaluate condition, then body, until condition is falsy.
Maximum 10,000 iterations (safety limit).

```tcl
set i 0
while {lt $i 10} {
    echo $i
    incr i
}
```

#### break

```
break
```

Exit the innermost `while` loop immediately.

#### continue

```
continue
```

Skip the rest of the current loop iteration and proceed to the next
condition evaluation.

#### exit

```
exit [code]
```

Terminate the interpreter. Optional integer exit code (default 0).

### Process Execution

#### pragma

```
pragma run-rc on
```

Enable structured `$rc` record for `run` results. Currently the only
supported pragma.

#### run

```
run program [args...]
```

Execute a child binary. When `pragma run-rc on` is active, sets the
`$rc` record with structured result information.

**$rc fields on success** (`$rc.run` = 0):
- `$rc.run` — 0
- `$rc.kind` — `"ok"`
- `$rc.prog` — program's exit code

**$rc fields on failure** (`$rc.run` > 0):
- `$rc.run` — error category: 1=not-found, 2=crash, 3=exec-failure, 4=usage-error
- `$rc.kind` — category string
- `$rc.err` — error code
- `$rc.msg` — human-readable message

Note: process execution is currently stubbed. All `run` invocations
return not-found until COR24 OS syscalls are defined.

### Filesystem Commands

All filesystem commands except `cd` and `pwd` are stubs awaiting COR24
OS syscall definitions. They return errors indicating the filesystem is
not available.

```
cd path          # change directory (works — tracks path internally)
pwd              # print working directory (works)
ls [path]        # list directory (stub)
mkdir path       # create directory (stub)
rm path          # remove file (stub)
mv src dst       # rename/move (stub)
cp src dst       # copy (stub)
stat path        # file info (stub)
fexists path     # file exists check (stub — always returns 0)
```

Paths starting with `/` are absolute. Other paths are resolved relative
to the current working directory.

### Source and Environment

#### source

```
source file.sws
```

Read and execute a script file line by line. Maximum nesting depth: 16.
Requires filesystem read support (currently stubbed).

#### env

```
env get NAME
env set NAME VALUE
env unset NAME
```

Get, set, or remove process environment variables. These are separate
from script variables (`set`/`$var`).

## Reserved Keywords

The following keywords are reserved for future use. Using them as
command names produces an error:

`try`, `catch`, `finally`, `throw`, `return`, `proc`, `for`, `foreach`

## Error Model

Commands return success (0) or error (-1). Errors print a message to
UART output prefixed with `error:`. After an error, the REPL continues
with the next line — errors do not terminate the session.

Special return signals for control flow:
- `break` — exit innermost loop
- `continue` — skip to next iteration

Undefined variable references and type mismatches are runtime errors.

## Static Limits

| Resource | Limit |
|----------|-------|
| Line length | 512 bytes |
| Tokens per line | 32 |
| Token length | 128 bytes |
| Variables | 64 |
| Values | 64 |
| Commands | 48 |
| Record fields | 16 per record |
| Source nesting | 16 levels |
| While iterations | 10,000 per loop |
| Command substitution nesting | 64 levels |
| Environment variables | 32 |
