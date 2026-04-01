Create docs/usage.md with comprehensive usage examples for sws.

This should be a practical, example-heavy guide organized by use case,
not a reference. Show how features combine to solve real tasks.

Sections:

1. Getting Started
   - Running sws on the emulator
   - Passing input via -u flag
   - The REPL prompt

2. Variables and Values
   - Setting and using variables
   - Integer vs string auto-detection
   - Overwriting variables
   - exists? for safe checking

3. Arithmetic and Expressions
   - Basic arithmetic (+, -, *, /, %)
   - Command substitution for inline math: (+ 1 2)
   - Nested expressions: (+ (* 3 4) 5)
   - Setting variables from expressions: set x (+ 1 2)
   - Using concat for string building (after step 013)

4. Control Flow
   - if/else with examples
   - while loops with counter pattern
   - break and continue
   - Nested loops
   - FizzBuzz as a complete example

5. Records and Structured Data
   - pragma run-rc on
   - Reading $rc fields after run
   - exists? on record fields
   - Pattern: check $rc.run before accessing other fields

6. Environment Variables
   - env set/get/unset
   - Using env values in expressions

7. Filesystem Commands
   - cd/pwd workflow
   - Note which commands are stubs

8. Patterns and Idioms
   - Accumulator pattern (set sum 0, while, incr)
   - Flag variables for status tracking
   - Error checking with exists? and if
   - Building output strings with concat

9. Limitations and Gotchas
   - No string interpolation in quotes
   - echo adds spaces between args
   - () not $() for command substitution
   - No semicolons outside blocks
   - Static limits (max vars, line length, etc.)

Each section should have 3-5 runnable examples with expected output.
Update README.md to link to docs/usage.md.