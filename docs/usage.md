# sws Usage Guide

A practical, example-driven guide to scripting with sws on COR24.

## Getting Started

### Running sws on the emulator

Build and launch the interpreter:

```bash
# Build sws from source
./scripts/build.sh

# Run interactively (REPL)
cor24-run sws.bin

# Run a script file via UART input
cor24-run sws.bin < myscript.sws

# Pass input directly with -u flag
cor24-run sws.bin -u 'echo "hello from sws"'
```

### The REPL prompt

When running interactively, sws reads one line at a time from UART
input and executes it immediately. There is no explicit prompt character
— type a command and press enter:

```tcl
echo hello
# output: hello

set x 42
echo $x
# output: 42
```

Errors print a message and return to the next line — they do not
terminate the session:

```tcl
echo $undefined
# output: error: undefined variable: undefined

echo "still running"
# output: still running
```

## Variables and Values

### Setting and using variables

```tcl
set name "COR24"
set version 1
echo $name $version
# output: COR24 1
```

### Integer vs string auto-detection

Values that look like integers (digits with optional leading minus)
are stored as integers. Everything else is a string:

```tcl
set x 42       # integer
set y -7       # integer
set z hello    # string
set w "123"    # integer (quotes don't force string type)
set v ""       # empty string
```

### Overwriting variables

`set` replaces the previous value, even if the type changes:

```tcl
set x 10
echo $x
# output: 10

set x "now a string"
echo $x
# output: now a string
```

### exists? for safe checking

Check whether a variable is defined without triggering an error:

```tcl
if {exists? $config} {
    echo "config is" $config
} else {
    echo "config not set"
}
# output: config not set

set config ready
if {exists? $config} {
    echo "config is" $config
}
# output: config is ready
```

`exists?` also works on record fields:

```tcl
pragma run-rc on
run something
if {exists? $rc.prog} {
    echo "exit code:" $rc.prog
} else {
    echo "no exit code (command failed)"
}
# output: no exit code (command failed)
```

## Arithmetic and Expressions

### Basic arithmetic

Arithmetic operators are commands that take two integer arguments:

```tcl
echo (+ 3 4)
# output: 7

echo (- 10 3)
# output: 7

echo (* 6 7)
# output: 42

echo (/ 15 4)
# output: 3

echo (% 17 5)
# output: 2
```

### Nested expressions

Command substitution nests naturally:

```tcl
echo (+ (* 3 4) 5)
# output: 17

echo (* (+ 1 2) (+ 3 4))
# output: 21

echo (+ (+ 1 2) (+ 3 (+ 4 5)))
# output: 15
```

### Setting variables from expressions

```tcl
set x (+ 1 2)
echo $x
# output: 3

set area (* 7 8)
echo $area
# output: 56

set sum (+ $x $area)
echo $sum
# output: 59
```

### Building strings with concat

`concat` joins arguments with no separator — useful for building
output strings:

```tcl
set name "world"
echo (concat "hello, " $name "!")
# output: hello, world!

set x 42
echo (concat "x=" $x)
# output: x=42

# Build a string from parts
set prefix "item"
set id 7
set label (concat $prefix "-" $id)
echo $label
# output: item-7
```

`concat` with no arguments returns an empty string:

```tcl
set empty (concat)
if {eq $empty ""} {
    echo "empty string"
}
# output: empty string
```

## Control Flow

### if/else

```tcl
set x 10
if {gt $x 5} {
    echo "big"
} else {
    echo "small"
}
# output: big
```

Chained conditions use nested if:

```tcl
set score 75
if {ge $score 90} {
    echo "A"
} else {
    if {ge $score 80} {
        echo "B"
    } else {
        if {ge $score 70} {
            echo "C"
        } else {
            echo "F"
        }
    }
}
# output: C
```

### while loops with counter pattern

```tcl
set i 0
while {lt $i 5} {
    echo $i
    incr i
}
# output:
# 0
# 1
# 2
# 3
# 4
```

Counting down:

```tcl
set i 3
while {gt $i 0} {
    echo (concat "T-" $i)
    incr i -1
}
echo "Go!"
# output:
# T-3
# T-2
# T-1
# Go!
```

### break and continue

Exit a loop early with `break`:

```tcl
set i 0
while {lt $i 100} {
    if {eq $i 3} {
        break
    }
    echo $i
    incr i
}
# output:
# 0
# 1
# 2
```

Skip iterations with `continue`:

```tcl
set i 0
while {lt $i 6} {
    incr i
    if {eq (% $i 2) 0} {
        continue
    }
    echo $i
}
# output:
# 1
# 3
# 5
```

### Nested loops

```tcl
set row 1
while {le $row 3} {
    set col 1
    while {le $col 3} {
        echo (concat $row "x" $col "=" (* $row $col))
        incr col
    }
    incr row
}
# output:
# 1x1=1
# 1x2=2
# 1x3=3
# 2x1=2
# 2x2=4
# 2x3=6
# 3x1=3
# 3x2=6
# 3x3=9
```

### FizzBuzz

A complete example combining arithmetic, control flow, and command
substitution:

```tcl
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
# output:
# 1
# 2
# Fizz
# 4
# Buzz
# Fizz
# 7
# 8
# Fizz
# Buzz
# 11
# Fizz
# 13
# 14
# FizzBuzz
```

## Records and Structured Data

### pragma run-rc on

Enable the structured `$rc` record before using `run`:

```tcl
pragma run-rc on
```

Without this pragma, `run` does not set `$rc`.

### Reading $rc fields after run

```tcl
pragma run-rc on

run some-program
echo "run status:" $rc.run
echo "category:" $rc.kind
```

On success (`$rc.run` = 0):

```tcl
pragma run-rc on
run my-tool
if {eq $rc.run 0} {
    echo "succeeded with exit code" $rc.prog
}
```

On failure (`$rc.run` > 0):

```tcl
pragma run-rc on
run nonexistent
echo $rc.run     # 1
echo $rc.kind    # not-found
echo $rc.msg     # binary not found
```

### exists? on record fields

Not all fields are present on every result. Check before accessing:

```tcl
pragma run-rc on
run something

if {exists? $rc.prog} {
    echo "exit code:" $rc.prog
}

if {exists? $rc.err} {
    echo "error:" $rc.err
}

if {exists? $rc.msg} {
    echo "message:" $rc.msg
}
```

### Pattern: check $rc.run before accessing other fields

```tcl
pragma run-rc on
run my-tool arg1 arg2

if {eq $rc.run 0} {
    echo "OK, exit code:" $rc.prog
} else {
    echo "Failed:" $rc.kind "-" $rc.msg
}
```

The `$rc.run` codes:
- `0` — ok (check `$rc.prog` for the program's exit code)
- `1` — not-found
- `2` — crash
- `3` — exec-failure
- `4` — usage-error

## Environment Variables

### env set/get/unset

Environment variables are separate from script variables:

```tcl
env set EDITOR vim
echo (env get EDITOR)
# output: vim

env set PATH "/bin:/usr/bin"
echo (env get PATH)
# output: /bin:/usr/bin

env unset EDITOR
```

### Using env values in expressions

```tcl
env set PORT 8080
set p (env get PORT)
echo "port:" $p
echo "next port:" (+ $p 1)
# output:
# port: 8080
# next port: 8081
```

### Checking environment state

```tcl
env set DEBUG 1

set debug_val (env get DEBUG)
if {eq $debug_val 1} {
    echo "debug mode on"
}
# output: debug mode on
```

## Filesystem Commands

### cd/pwd workflow

`cd` and `pwd` work by tracking paths internally:

```tcl
echo (pwd)
# output: /

cd /home/user
echo (pwd)
# output: /home/user

cd projects
echo (pwd)
# output: /home/user/projects

cd ..
echo (pwd)
# output: /home/user
```

### Stub commands

The following filesystem commands are defined but return errors until
COR24 OS syscalls are implemented:

- `ls [path]` — list directory
- `mkdir path` — create directory
- `rm path` — remove file
- `mv src dst` — rename/move
- `cp src dst` — copy file
- `stat path` — file info
- `fexists path` — always returns 0

```tcl
ls /home
# output: error: filesystem not available

fexists /bin/sh
# output: 0
```

These stubs exist so scripts can be written now and will work when
the filesystem becomes available.

## Patterns and Idioms

### Accumulator pattern

Sum numbers in a loop:

```tcl
set sum 0
set i 1
while {le $i 10} {
    set sum (+ $sum (* $i $i))
    incr i
}
echo "Sum of squares 1-10:" $sum
# output: Sum of squares 1-10: 385
```

Count occurrences:

```tcl
set count 0
set i 1
while {le $i 20} {
    if {eq (% $i 3) 0} {
        incr count
    }
    incr i
}
echo "Multiples of 3 up to 20:" $count
# output: Multiples of 3 up to 20: 6
```

### Flag variables for status tracking

```tcl
set found 0
set i 1
while {le $i 100} {
    if {eq (* $i $i) 144} {
        set found 1
        set answer $i
        break
    }
    incr i
}
if {eq $found 1} {
    echo "sqrt(144) =" $answer
} else {
    echo "not found"
}
# output: sqrt(144) = 12
```

### Error checking with exists? and if

```tcl
pragma run-rc on

# Run a build step, bail on failure
run tc24r main.c -o main.s
if {ne $rc.run 0} {
    echo "compile failed:" $rc.msg
    exit 1
}

# Safe field access
if {exists? $rc.prog} {
    if {ne $rc.prog 0} {
        echo "compiler returned error code" $rc.prog
        exit 1
    }
}
echo "compile OK"
```

### Building output strings with concat

```tcl
# Build a formatted status line
set step 3
set total 5
set status "OK"
echo (concat "[" $step "/" $total "] " $status)
# output: [3/5] OK

# Accumulate a string in a loop
set result ""
set i 1
while {le $i 5} {
    if {gt (+ 0 (eq $result "")) 0} {
        set result (concat $i)
    } else {
        set result (concat $result "," $i)
    }
    incr i
}
echo $result
# output: 1,2,3,4,5
```

### Multi-step build script pattern

```tcl
pragma run-rc on
set errors 0
set step 0

# Step 1
incr step
echo (concat "[" $step "] compile")
run tc24r main.c -o main.s
if {ne $rc.run 0} {
    echo "  FAIL:" $rc.msg
    incr errors
} else {
    echo "  OK"
}

# Step 2
incr step
echo (concat "[" $step "] assemble")
run as24 main.s -o main.bin
if {ne $rc.run 0} {
    echo "  FAIL:" $rc.msg
    incr errors
} else {
    echo "  OK"
}

# Summary
echo ""
if {eq $errors 0} {
    echo (concat "Build OK (" $step " steps)")
} else {
    echo (concat "Build FAILED (" $errors " error(s))")
    exit 1
}
```

## Limitations and Gotchas

### No string interpolation in quotes

Quoted strings are literal — `$var` is not expanded inside quotes:

```tcl
set x 42
echo "$x"       # output: $x      (literal, NOT 42)
echo $x          # output: 42      (correct)
echo "val:" $x   # output: val: 42 (separate args)
```

Use `concat` to build strings with embedded values:

```tcl
set name "world"
echo (concat "hello, " $name)
# output: hello, world
```

### echo adds spaces between args

`echo` joins all arguments with a single space:

```tcl
echo "a" "b" "c"
# output: a b c

# To output with no spaces, use concat:
echo (concat "a" "b" "c")
# output: abc
```

### () not $() for command substitution

sws uses parentheses, not the `$(...)` syntax found in Unix shells:

```tcl
# Correct:
set x (+ 1 2)

# Wrong — this is a syntax error:
# set x $(+ 1 2)
```

### No semicolons outside blocks

Each command goes on its own line. Semicolons are not command
separators:

```tcl
# Correct:
set x 1
set y 2

# Wrong — the semicolon becomes part of the value:
# set x 1; set y 2
```

Inside blocks, each command is on its own line:

```tcl
if {eq 1 1} {
    echo "a"
    echo "b"
}
```

### Static limits

sws uses fixed-size buffers with no dynamic allocation. Be aware of
these limits:

| Resource | Limit |
|----------|-------|
| Line length | 512 bytes |
| Tokens per line | 32 |
| Token length | 128 bytes |
| Variables | 64 |
| Commands | 48 |
| Record fields | 16 per record |
| Source nesting | 16 levels |
| While iterations | 10,000 per loop |
| Command substitution nesting | 64 levels |
| Environment variables | 32 |

Exceeding a limit produces a runtime error — the interpreter does not
crash.

### Integer range

Integers are 24-bit signed: -8388608 to 8388607. Overflow wraps
silently.
