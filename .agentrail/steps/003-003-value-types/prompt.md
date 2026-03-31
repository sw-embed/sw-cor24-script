Implement the Value type system for sws.

sws has three value types: Integer, String, and Record. Since tc24r has
no structs, implement using parallel arrays and type tags.

1. Value type tags:
   - VAL_INT = 0
   - VAL_STR = 1
   - VAL_REC = 2 (record/map — used for $rc)

2. Value storage (parallel arrays):
   - val_type[MAX_VALS] — type tag
   - val_int[MAX_VALS] — integer value (when type == VAL_INT)
   - val_str[MAX_VALS][MAX_VAR_VAL] — string value (when type == VAL_STR)
   - For records: val_rec_fields[MAX_VALS][MAX_FIELDS] — field name indices
   - val_rec_values[MAX_VALS][MAX_FIELDS] — field value indices
   - val_rec_count[MAX_VALS] — number of fields in record

3. Value operations:
   - val_new_int(n) → allocate and return value index
   - val_new_str(s) → allocate and return value index
   - val_new_rec() → allocate empty record, return index
   - val_rec_set(rec_idx, field_name, val_idx) → set field in record
   - val_rec_get(rec_idx, field_name) → get field value index, or -1
   - val_rec_has(rec_idx, field_name) → 1 if field exists, 0 if not
   - val_to_str(idx, buf) → format any value as string for output
   - val_eq(a, b) → compare two values
   - val_is_truthy(idx) → nonzero int = true, nonempty string = true

4. Value printing:
   - Print integers as decimal
   - Print strings as-is
   - Print records as debug representation

5. Error model:
   - All value operations should return success/failure indicators
   - Map to Result<Value, ScriptError> conceptually
   - Track allocation count, fail on overflow

Test: create values of each type, print them via UART, verify output.

Reference: docs/research.txt (value types, record model for $rc)