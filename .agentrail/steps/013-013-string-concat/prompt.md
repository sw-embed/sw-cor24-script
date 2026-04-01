Add string concatenation command to sws.

1. Implement `concat` command:
   - `concat arg1 arg2 ...` — concatenate all arguments into a single string with no separators
   - Returns the concatenated string as the command result
   - Example: `concat "x=" (+ 1 2)` → `"x=3"`
   - Example: `set name "world"` then `concat "hello, " $name "!"` → `"hello, world!"`

2. Add tests:
   - Basic two-string concat
   - Concat with integer (auto-converts to string)
   - Concat with command substitution
   - Concat with variables
   - Concat single arg (identity)
   - Concat no args (empty string)
   - Use in echo: `echo (concat "x=" (+ 1 2))` prints `x=3`
   - Use in set: `set msg (concat "hello " $name)`

3. Update documentation:
   - Add concat to README.md command list
   - Add concat to docs/language-reference.md
   - Update CHANGES.md