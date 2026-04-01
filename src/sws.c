/* sws.c -- Software Wrighter Script interpreter for COR24
 *
 * A Tcl-like command language for shell and editor scripting.
 * Compiled by tc24r (cross C compiler), runs on COR24 hardware or emulator.
 *
 * tc24r constraints: no structs, no malloc/free, no string library,
 * no varargs/printf, single translation unit, 24-bit int, 8-bit char.
 */

/* ---- UART I/O (memory-mapped) ---- */

#define UART_DATA   0xFF0100
#define UART_STATUS 0xFF0101

void uart_putc(int ch) {
    int tries = 50000;
    while ((*(char *)UART_STATUS & 0x80) && tries > 0) {
        tries = tries - 1;
    }
    *(char *)UART_DATA = ch;
}

int uart_getc() {
    while (!(*(char *)UART_STATUS & 0x01)) {}
    return *(char *)UART_DATA;
}

void uart_puts(char *s) {
    while (*s) {
        uart_putc(*s);
        s = s + 1;
    }
}

void uart_putint(int n) {
    if (n < 0) {
        uart_putc(45); /* '-' */
        n = 0 - n;
    }
    if (n == 0) {
        uart_putc(48); /* '0' */
        return;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i] = 48 + n % 10;
        n = n / 10;
        i = i + 1;
    }
    while (i > 0) {
        i = i - 1;
        uart_putc(buf[i]);
    }
}

void uart_newline() {
    uart_putc(10);
}

/* ---- Static allocation limits ---- */

#define MAX_LINE_LEN   512
#define MAX_TOKENS       32
#define MAX_TOKEN_LEN   128
#define MAX_VARS         64
#define MAX_VAR_NAME     32
#define MAX_VAR_VAL     128
#define MAX_NEST         64
#define MAX_FIELDS       16

/* ---- String helpers ---- */

int str_len(char *s) {
    int n = 0;
    while (s[n]) n = n + 1;
    return n;
}

int str_eq(char *a, char *b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i = i + 1;
    }
    return a[i] == b[i];
}

void str_copy(char *dst, char *src) {
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

int str_starts_with(char *s, char *prefix) {
    int i = 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i = i + 1;
    }
    return 1;
}

int str_to_int(char *s) {
    int n = 0;
    int neg = 0;
    int i = 0;
    if (s[0] == 45) { /* '-' */
        neg = 1;
        i = 1;
    }
    while (s[i] >= 48 && s[i] <= 57) { /* '0'-'9' */
        n = n * 10 + (s[i] - 48);
        i = i + 1;
    }
    if (neg) n = 0 - n;
    return n;
}

void int_to_str(char *buf, int n) {
    if (n == 0) {
        buf[0] = 48;
        buf[1] = 0;
        return;
    }
    int neg = 0;
    if (n < 0) {
        neg = 1;
        n = 0 - n;
    }
    char tmp[12];
    int i = 0;
    while (n > 0) {
        tmp[i] = 48 + n % 10;
        n = n / 10;
        i = i + 1;
    }
    int j = 0;
    if (neg) {
        buf[0] = 45;
        j = 1;
    }
    while (i > 0) {
        i = i - 1;
        buf[j] = tmp[i];
        j = j + 1;
    }
    buf[j] = 0;
}

/* ---- Tokenizer ---- */

/* Token types */
#define TOK_WORD   0
#define TOK_QUOTED 1
#define TOK_BLOCK  2

/* Flat buffer: token i starts at token_starts[i] in token_pool */
#define TOKEN_POOL_SIZE 2048
char token_pool[TOKEN_POOL_SIZE];
int  token_starts[MAX_TOKENS];
int  token_type[MAX_TOKENS];
int  token_count;
int  token_pool_used;

/* Get pointer to token string */
char *token_str(int idx) {
    return token_pool + token_starts[idx];
}

/* Append a char to the current token being built */
void tok_putc(int ch) {
    if (token_pool_used < TOKEN_POOL_SIZE - 1) {
        token_pool[token_pool_used] = ch;
        token_pool_used = token_pool_used + 1;
    }
}

/* Terminate the current token */
void tok_end() {
    if (token_pool_used < TOKEN_POOL_SIZE) {
        token_pool[token_pool_used] = 0;
        token_pool_used = token_pool_used + 1;
    }
}

/* tokenize_line: split a line into tokens.
 * Returns token count, or -1 on syntax error.
 */
int tokenize_line(char *line) {
    int i = 0;
    token_count = 0;
    token_pool_used = 0;

    while (line[i]) {
        /* skip whitespace */
        while (line[i] == 32 || line[i] == 9)
            i = i + 1;

        if (line[i] == 0) break;

        /* comment — rest of line ignored */
        if (line[i] == 35) break;

        if (token_count >= MAX_TOKENS) break;

        if (line[i] == 34) { /* '"' — quoted string */
            token_starts[token_count] = token_pool_used;
            i = i + 1; /* skip opening quote */
            while (line[i] && line[i] != 34) {
                if (line[i] == 92 && line[i + 1]) { /* backslash escape */
                    i = i + 1;
                    if (line[i] == 110)
                        tok_putc(10); /* \n */
                    else if (line[i] == 116)
                        tok_putc(9); /* \t */
                    else
                        tok_putc(line[i]);
                    i = i + 1;
                } else {
                    tok_putc(line[i]);
                    i = i + 1;
                }
            }
            if (line[i] != 34) {
                uart_puts("error: unterminated quote");
                uart_newline();
                return -1;
            }
            i = i + 1; /* skip closing quote */
            tok_end();
            token_type[token_count] = TOK_QUOTED;
            token_count = token_count + 1;

        } else if (line[i] == 123) { /* '{' — brace block */
            token_starts[token_count] = token_pool_used;
            int depth = 1;
            tok_putc(line[i]); /* opening brace */
            i = i + 1;
            while (line[i] && depth > 0) {
                if (line[i] == 123) depth = depth + 1;
                if (line[i] == 125) depth = depth - 1;
                tok_putc(line[i]);
                i = i + 1;
            }
            if (depth != 0) {
                uart_puts("error: unterminated brace");
                uart_newline();
                return -1;
            }
            tok_end();
            token_type[token_count] = TOK_BLOCK;
            token_count = token_count + 1;

        } else { /* bare word */
            token_starts[token_count] = token_pool_used;
            while (line[i] && line[i] != 32 && line[i] != 9
                   && line[i] != 35) {
                tok_putc(line[i]);
                i = i + 1;
            }
            tok_end();
            token_type[token_count] = TOK_WORD;
            token_count = token_count + 1;
        }
    }

    return token_count;
}

/* ---- Value type system ---- */

/* Type tags */
#define VAL_INT 0
#define VAL_STR 1
#define VAL_REC 2

/* Value storage limits */
#define MAX_VALS       64

/* Parallel arrays for value storage.
 * 2D arrays are flattened to 1D to avoid large strides that
 * exceed tc24r/as24's lc immediate range (0..127). */
int  val_type[MAX_VALS];
int  val_int[MAX_VALS];
/* 64 * 128 = 8192 */
#define VAL_STR_POOL_SIZE 8192
/* 64 * 16 = 1024 */
#define VAL_REC_POOL_SIZE 1024

char val_str_pool[VAL_STR_POOL_SIZE];          /* flat string pool */
int  val_rec_fld[VAL_REC_POOL_SIZE];           /* field name value indices */
int  val_rec_val[VAL_REC_POOL_SIZE];           /* field value indices */
int  val_rec_count[MAX_VALS];
int  val_count;

/* Index helpers for flat arrays */
int vsp(int idx) { return idx * MAX_VAR_VAL; }  /* val_str_pool offset */
int vrf(int idx, int f) { return idx * MAX_FIELDS + f; } /* rec field offset */

/* Get pointer to a value's string data */
char *val_str_ptr(int idx) {
    return val_str_pool + vsp(idx);
}

/* Reset all values (call at start or between scripts) */
void val_reset() {
    val_count = 0;
}

/* Allocate a new integer value. Returns index, or -1 on overflow. */
int val_new_int(int n) {
    if (val_count >= MAX_VALS) return -1;
    int idx = val_count;
    val_type[idx] = VAL_INT;
    val_int[idx] = n;
    val_count = val_count + 1;
    return idx;
}

/* Allocate a new string value. Returns index, or -1 on overflow. */
int val_new_str(char *s) {
    if (val_count >= MAX_VALS) return -1;
    int idx = val_count;
    val_type[idx] = VAL_STR;
    /* Copy string, truncate if needed */
    char *dst = val_str_ptr(idx);
    int i = 0;
    while (s[i] && i < MAX_VAR_VAL - 1) {
        dst[i] = s[i];
        i = i + 1;
    }
    dst[i] = 0;
    val_count = val_count + 1;
    return idx;
}

/* Allocate a new empty record. Returns index, or -1 on overflow. */
int val_new_rec() {
    if (val_count >= MAX_VALS) return -1;
    int idx = val_count;
    val_type[idx] = VAL_REC;
    val_rec_count[idx] = 0;
    val_count = val_count + 1;
    return idx;
}

/* Set a field in a record. field_name_idx and val_idx are value indices.
 * Returns 0 on success, -1 on error. */
int val_rec_set(int rec_idx, int field_name_idx, int val_idx) {
    if (rec_idx < 0 || rec_idx >= val_count) return -1;
    if (val_type[rec_idx] != VAL_REC) return -1;

    /* Check if field already exists — update in place */
    int fc = val_rec_count[rec_idx];
    int i = 0;
    while (i < fc) {
        if (val_rec_fld[vrf(rec_idx, i)] == field_name_idx) {
            val_rec_val[vrf(rec_idx, i)] = val_idx;
            return 0;
        }
        i = i + 1;
    }

    /* Add new field */
    if (fc >= MAX_FIELDS) return -1;
    val_rec_fld[vrf(rec_idx, fc)] = field_name_idx;
    val_rec_val[vrf(rec_idx, fc)] = val_idx;
    val_rec_count[rec_idx] = fc + 1;
    return 0;
}

/* Get a field value from a record by field name string.
 * Returns value index, or -1 if not found. */
int val_rec_get(int rec_idx, char *field_name) {
    if (rec_idx < 0 || rec_idx >= val_count) return -1;
    if (val_type[rec_idx] != VAL_REC) return -1;

    int fc = val_rec_count[rec_idx];
    int i = 0;
    while (i < fc) {
        int fn_idx = val_rec_fld[vrf(rec_idx, i)];
        if (val_type[fn_idx] == VAL_STR && str_eq(val_str_ptr(fn_idx), field_name)) {
            return val_rec_val[vrf(rec_idx, i)];
        }
        i = i + 1;
    }
    return -1;
}

/* Check if a record has a field. Returns 1 if exists, 0 if not. */
int val_rec_has(int rec_idx, char *field_name) {
    return val_rec_get(rec_idx, field_name) >= 0;
}

/* Format a value as a string into buf. Returns buf. */
char *val_to_str(int idx, char *buf) {
    if (idx < 0 || idx >= val_count) {
        buf[0] = 0;
        return buf;
    }
    if (val_type[idx] == VAL_INT) {
        int_to_str(buf, val_int[idx]);
    } else if (val_type[idx] == VAL_STR) {
        str_copy(buf, val_str_ptr(idx));
    } else if (val_type[idx] == VAL_REC) {
        /* Debug representation: {field1: val1, field2: val2} */
        int p = 0;
        buf[p] = 123; /* '{' */
        p = p + 1;
        int fc = val_rec_count[idx];
        int i = 0;
        while (i < fc && p < MAX_VAR_VAL - 10) {
            if (i > 0) {
                buf[p] = 44; /* ',' */
                p = p + 1;
                buf[p] = 32; /* ' ' */
                p = p + 1;
            }
            /* field name */
            int fn_idx = val_rec_fld[vrf(idx, i)];
            char tmp[MAX_VAR_VAL];
            val_to_str(fn_idx, tmp);
            int k = 0;
            while (tmp[k] && p < MAX_VAR_VAL - 5) {
                buf[p] = tmp[k];
                p = p + 1;
                k = k + 1;
            }
            buf[p] = 58; /* ':' */
            p = p + 1;
            buf[p] = 32; /* ' ' */
            p = p + 1;
            /* field value */
            int fv_idx = val_rec_val[vrf(idx, i)];
            val_to_str(fv_idx, tmp);
            k = 0;
            while (tmp[k] && p < MAX_VAR_VAL - 3) {
                buf[p] = tmp[k];
                p = p + 1;
                k = k + 1;
            }
            i = i + 1;
        }
        buf[p] = 125; /* '}' */
        p = p + 1;
        buf[p] = 0;
    } else {
        buf[0] = 0;
    }
    return buf;
}

/* Compare two values for equality. Returns 1 if equal, 0 if not. */
int val_eq(int a, int b) {
    if (a < 0 || a >= val_count || b < 0 || b >= val_count) return 0;
    if (val_type[a] != val_type[b]) return 0;
    if (val_type[a] == VAL_INT) {
        return val_int[a] == val_int[b];
    }
    if (val_type[a] == VAL_STR) {
        return str_eq(val_str_ptr(a), val_str_ptr(b));
    }
    /* Records: equal only if same index */
    return a == b;
}

/* Truthiness: nonzero int = true, nonempty string = true,
 * record = true, invalid = false. */
int val_is_truthy(int idx) {
    if (idx < 0 || idx >= val_count) return 0;
    if (val_type[idx] == VAL_INT) {
        return val_int[idx] != 0;
    }
    if (val_type[idx] == VAL_STR) {
        char *sp = val_str_ptr(idx);
        return sp[0] != 0;
    }
    /* Records are always truthy */
    return 1;
}

/* ---- Variable environment ---- */

/* Flat name pool: var i's name starts at var_name_pool[i * MAX_VAR_NAME] */
#define VAR_NAME_POOL_SIZE 2048  /* 64 * 32 = 2048 */
char var_name_pool[VAR_NAME_POOL_SIZE];
int  var_val[MAX_VARS];   /* value index into value storage */
int  var_count;

/* Get pointer to variable name */
char *var_name(int idx) {
    return var_name_pool + idx * MAX_VAR_NAME;
}

/* Find variable index by name. Returns -1 if not found. */
int env_find(char *name) {
    int i = 0;
    while (i < var_count) {
        if (str_eq(var_name(i), name)) return i;
        i = i + 1;
    }
    return -1;
}

/* Set or update a variable. Returns 0 on success, -1 on overflow. */
int env_set(char *name, int val_idx) {
    int i = env_find(name);
    if (i >= 0) {
        var_val[i] = val_idx;
        return 0;
    }
    if (var_count >= MAX_VARS) return -1;
    /* Copy name into pool */
    char *dst = var_name(var_count);
    int k = 0;
    while (name[k] && k < MAX_VAR_NAME - 1) {
        dst[k] = name[k];
        k = k + 1;
    }
    dst[k] = 0;
    var_val[var_count] = val_idx;
    var_count = var_count + 1;
    return 0;
}

/* Get value index for a variable. Returns -1 if undefined. */
int env_get(char *name) {
    int i = env_find(name);
    if (i < 0) return -1;
    return var_val[i];
}

/* Check if variable exists. Returns 1 if yes, 0 if no. */
int env_has(char *name) {
    return env_find(name) >= 0;
}

/* Remove a variable. Returns 0 on success, -1 if not found. */
int env_unset(char *name) {
    int i = env_find(name);
    if (i < 0) return -1;
    /* Shift remaining vars down */
    while (i < var_count - 1) {
        str_copy(var_name(i), var_name(i + 1));
        var_val[i] = var_val[i + 1];
        i = i + 1;
    }
    var_count = var_count - 1;
    return 0;
}

/* Reset environment */
void env_reset() {
    var_count = 0;
}

/* ---- Variable substitution ---- */

/* Expanded token buffer — tokens after $var substitution */
#define EXPAND_POOL_SIZE 2048
char expand_pool[EXPAND_POOL_SIZE];
int  expand_starts[MAX_TOKENS];
int  expand_count;
int  expand_pool_used;

/* Parse a variable reference from a token string starting at $.
 * Writes var name into vname, field name into fname (or empty).
 * Returns number of chars consumed (not counting the $). */
int parse_var_ref(char *s, char *vname, char *fname) {
    int i = 0;
    int vi = 0;
    /* Read variable name: alphanumeric and underscore */
    while (s[i] && s[i] != 46 /* '.' */ && s[i] != 32 && s[i] != 9
           && vi < MAX_VAR_NAME - 1) {
        vname[vi] = s[i];
        vi = vi + 1;
        i = i + 1;
    }
    vname[vi] = 0;
    fname[0] = 0;
    /* Check for .field */
    if (s[i] == 46) { /* '.' */
        i = i + 1;
        int fi = 0;
        while (s[i] && s[i] != 32 && s[i] != 9
               && fi < MAX_VAR_NAME - 1) {
            fname[fi] = s[i];
            fi = fi + 1;
            i = i + 1;
        }
        fname[fi] = 0;
    }
    return i;
}

/* Runtime error flag — set to 1 on error */
int runtime_error;

/* Expand variables in tokens. Replaces token_pool/token_starts with
 * expanded versions in expand_pool/expand_starts.
 * Returns 0 on success, -1 on error (e.g. undefined variable). */
int expand_variables() {
    expand_count = 0;
    expand_pool_used = 0;
    runtime_error = 0;
    int t = 0;
    while (t < token_count) {
        char *tok = token_str(t);
        int ttype = token_type[t];
        expand_starts[expand_count] = expand_pool_used;

        if (ttype == TOK_BLOCK) {
            /* Blocks are NOT expanded — they are deferred */
            int k = 0;
            while (tok[k] && expand_pool_used < EXPAND_POOL_SIZE - 1) {
                expand_pool[expand_pool_used] = tok[k];
                expand_pool_used = expand_pool_used + 1;
                k = k + 1;
            }
        } else if (tok[0] == 36) { /* '$' — entire token is a variable ref */
            char vname[MAX_VAR_NAME];
            char fname[MAX_VAR_NAME];
            parse_var_ref(tok + 1, vname, fname);

            int vi = env_get(vname);
            if (vi < 0) {
                uart_puts("error: undefined variable: ");
                uart_puts(vname);
                uart_newline();
                runtime_error = 1;
                return -1;
            }

            if (fname[0]) {
                /* $var.field — record field access */
                if (val_type[vi] != VAL_REC) {
                    uart_puts("error: not a record: ");
                    uart_puts(vname);
                    uart_newline();
                    runtime_error = 1;
                    return -1;
                }
                int fvi = val_rec_get(vi, fname);
                if (fvi < 0) {
                    uart_puts("error: no field '");
                    uart_puts(fname);
                    uart_puts("' in ");
                    uart_puts(vname);
                    uart_newline();
                    runtime_error = 1;
                    return -1;
                }
                /* Format field value as string */
                char tmp[MAX_VAR_VAL];
                val_to_str(fvi, tmp);
                int k = 0;
                while (tmp[k] && expand_pool_used < EXPAND_POOL_SIZE - 1) {
                    expand_pool[expand_pool_used] = tmp[k];
                    expand_pool_used = expand_pool_used + 1;
                    k = k + 1;
                }
            } else {
                /* $var — format value as string */
                char tmp[MAX_VAR_VAL];
                val_to_str(vi, tmp);
                int k = 0;
                while (tmp[k] && expand_pool_used < EXPAND_POOL_SIZE - 1) {
                    expand_pool[expand_pool_used] = tmp[k];
                    expand_pool_used = expand_pool_used + 1;
                    k = k + 1;
                }
            }
        } else {
            /* Plain token — copy as-is */
            int k = 0;
            while (tok[k] && expand_pool_used < EXPAND_POOL_SIZE - 1) {
                expand_pool[expand_pool_used] = tok[k];
                expand_pool_used = expand_pool_used + 1;
                k = k + 1;
            }
        }
        /* Null-terminate expanded token */
        if (expand_pool_used < EXPAND_POOL_SIZE) {
            expand_pool[expand_pool_used] = 0;
            expand_pool_used = expand_pool_used + 1;
        }
        expand_count = expand_count + 1;
        t = t + 1;
    }
    return 0;
}

/* Get pointer to expanded token string */
char *exp_str(int idx) {
    return expand_pool + expand_starts[idx];
}

/* ---- Helpers for value creation from token strings ---- */

/* Check if a string looks like an integer (optional minus, then digits) */
int is_int_str(char *s) {
    int i = 0;
    if (s[0] == 45) i = 1; /* '-' */
    if (s[i] == 0) return 0; /* empty or just "-" */
    while (s[i]) {
        if (s[i] < 48 || s[i] > 57) return 0;
        i = i + 1;
    }
    return 1;
}

/* Create a value from a string — auto-detect int vs string */
int val_from_str(char *s) {
    if (is_int_str(s)) {
        return val_new_int(str_to_int(s));
    }
    return val_new_str(s);
}

/* ---- Command dispatch table ---- */

#define MAX_CMDS      32
#define MAX_CMD_NAME  16
#define CMD_NAME_POOL_SIZE 512  /* 32 * 16 */

char cmd_name_pool[CMD_NAME_POOL_SIZE];
int  cmd_handler[MAX_CMDS];  /* function pointers stored as int (24-bit = ptr size) */
int  cmd_count;

/* Get pointer to command name */
char *cmd_name(int idx) {
    return cmd_name_pool + idx * MAX_CMD_NAME;
}

/* Register a command. Handler is a function pointer stored as int
 * (on COR24, pointers and ints are both 24-bit). */
int cmd_register(char *name, int handler) {
    if (cmd_count >= MAX_CMDS) return -1;
    char *dst = cmd_name(cmd_count);
    int i = 0;
    while (name[i] && i < MAX_CMD_NAME - 1) {
        dst[i] = name[i];
        i = i + 1;
    }
    dst[i] = 0;
    cmd_handler[cmd_count] = handler;
    cmd_count = cmd_count + 1;
    return 0;
}

/* Look up command by name. Returns index, or -1 if not found. */
int cmd_find(char *name) {
    int i = 0;
    while (i < cmd_count) {
        if (str_eq(cmd_name(i), name)) return i;
        i = i + 1;
    }
    return -1;
}

/* ---- Return signals ---- */

#define RET_OK        0
#define RET_ERR      -1
#define RET_BREAK    -2
#define RET_CONTINUE -3

int last_result; /* value index of last command/expression result */

/* ---- Built-in command handlers ---- */

/* All handlers access expanded tokens via exp_str()/expand_count.
 * Return 0 on success, -1 on error. */

int cmd_echo() {
    int j = 1;
    while (j < expand_count) {
        if (j > 1) uart_putc(32);
        uart_puts(exp_str(j));
        j = j + 1;
    }
    uart_newline();
    return 0;
}

int cmd_set() {
    if (expand_count < 3) {
        uart_puts("error: set: expected 2 args, got ");
        uart_putint(expand_count - 1);
        uart_newline();
        return -1;
    }
    char *name = exp_str(1);
    char *val_s = exp_str(2);
    int vi = val_from_str(val_s);
    if (vi < 0) {
        uart_puts("error: value overflow");
        uart_newline();
        return -1;
    }
    env_set(name, vi);
    return 0;
}

/* Exit flag — checked by main loop */
int exit_flag;
int exit_code;

/* ---- Pragma flags ---- */

int pragma_run_rc;  /* 0 = off (default), 1 = on */
int rc_defined;     /* 0 = $rc not yet set, 1 = has been set by a run */

int cmd_exit() {
    exit_flag = 1;
    exit_code = 0;
    if (expand_count >= 2) {
        exit_code = str_to_int(exp_str(1));
    }
    return 0;
}

/* ---- Input buffer ---- */

char line_buf[MAX_LINE_LEN];

int read_line(char *buf, int max) {
    int i = 0;
    int brace_depth = 0;
    while (i < max - 1) {
        int ch = uart_getc();
        if (ch == 4) { /* Ctrl-D: EOF */
            if (i == 0) return -1;
            break;
        }
        if (ch == 10 || ch == 13) { /* newline */
            if (brace_depth <= 0) break;
            /* Inside braces: treat newline as newline in block */
            buf[i] = 10;
            i = i + 1;
            continue;
        }
        buf[i] = ch;
        i = i + 1;
        if (ch == 123) brace_depth = brace_depth + 1; /* '{' */
        if (ch == 125) brace_depth = brace_depth - 1; /* '}' */
    }
    buf[i] = 0;
    return i;
}

/* ---- Halt ---- */

void halt() {
    asm("_user_halt:");
    asm("bra _user_halt");
}

/* ---- Main ---- */

char *tok_type_name(int t) {
    if (t == TOK_WORD)   return "WORD";
    if (t == TOK_QUOTED) return "QUOTED";
    if (t == TOK_BLOCK)  return "BLOCK";
    return "?";
}

char val_fmt_buf[MAX_VAR_VAL];

void val_print(int idx) {
    val_to_str(idx, val_fmt_buf);
    uart_puts(val_fmt_buf);
}

/* ---- exists? helper (checks without runtime error) ---- */

/* Check if $var or $var.field reference is valid.
 * tok is the raw token including the $ prefix.
 * Returns 1 if present, 0 if not. Never errors. */
int check_exists(char *tok) {
    if (tok[0] != 36) return 0; /* must start with $ */
    char vname[MAX_VAR_NAME];
    char fname[MAX_VAR_NAME];
    parse_var_ref(tok + 1, vname, fname);

    int vi = env_get(vname);
    if (vi < 0) return 0; /* variable not defined */
    if (fname[0] == 0) return 1; /* just $var, exists */
    /* $var.field — check record */
    if (val_type[vi] != VAL_REC) return 0;
    return val_rec_has(vi, fname);
}

/* ---- _toktest handler (debug: dump tokens) ---- */

int cmd_toktest() {
    /* Print expanded tokens in debug format */
    int j = 0;
    while (j < expand_count) {
        uart_puts("[");
        uart_puts(tok_type_name(token_type[j]));
        uart_puts("|");
        uart_puts(exp_str(j));
        uart_puts("]");
        if (j < expand_count - 1) uart_putc(32);
        j = j + 1;
    }
    uart_newline();
    return 0;
}

/* ---- _valtest handler (debug/testing) ---- */

int cmd_valtest() {
    val_reset();

    /* Integer values */
    int v0 = val_new_int(42);
    int v1 = val_new_int(0);
    int v2 = val_new_int(-7);

    /* String values */
    int v3 = val_new_str("hello");
    int v4 = val_new_str("");

    /* Record with fields */
    int v5 = val_new_rec();
    int fn_status = val_new_str("status");
    int fv_status = val_new_int(0);
    int fn_output = val_new_str("output");
    int fv_output = val_new_str("ok");
    val_rec_set(v5, fn_status, fv_status);
    val_rec_set(v5, fn_output, fv_output);

    /* Print each value */
    uart_puts("int:"); val_print(v0); uart_newline();
    uart_puts("zero:"); val_print(v1); uart_newline();
    uart_puts("neg:"); val_print(v2); uart_newline();
    uart_puts("str:"); val_print(v3); uart_newline();
    uart_puts("empty:"); val_print(v4); uart_newline();
    uart_puts("rec:"); val_print(v5); uart_newline();

    /* Equality tests */
    int v6 = val_new_int(42);
    uart_puts("eq:"); uart_putint(val_eq(v0, v6)); uart_newline();
    uart_puts("ne:"); uart_putint(val_eq(v0, v1)); uart_newline();

    /* Truthiness */
    uart_puts("truthy42:"); uart_putint(val_is_truthy(v0)); uart_newline();
    uart_puts("truthy0:"); uart_putint(val_is_truthy(v1)); uart_newline();
    uart_puts("truthyS:"); uart_putint(val_is_truthy(v3)); uart_newline();
    uart_puts("truthyE:"); uart_putint(val_is_truthy(v4)); uart_newline();
    uart_puts("truthyR:"); uart_putint(val_is_truthy(v5)); uart_newline();

    /* Record field access */
    int got = val_rec_get(v5, "status");
    uart_puts("field:"); val_print(got); uart_newline();
    uart_puts("has:"); uart_putint(val_rec_has(v5, "output")); uart_newline();
    uart_puts("miss:"); uart_putint(val_rec_has(v5, "nope")); uart_newline();

    uart_puts("vals:"); uart_putint(val_count); uart_newline();
    return 0;
}

/* ---- exists? handler (pre-expansion command) ---- */

int cmd_exists() {
    /* Uses raw tokens (before expansion) — token_str/token_count */
    if (token_count < 2) {
        uart_puts("error: exists?: expected 1 arg, got 0");
        uart_newline();
        return RET_ERR;
    }
    int result = check_exists(token_str(1));
    last_result = val_new_int(result);
    uart_putint(result);
    uart_newline();
    return RET_OK;
}

/* ---- Reserved keyword check ---- */

int is_reserved(char *name) {
    if (str_eq(name, "try")) return 1;
    if (str_eq(name, "catch")) return 1;
    if (str_eq(name, "finally")) return 1;
    if (str_eq(name, "throw")) return 1;
    if (str_eq(name, "return")) return 1;
    if (str_eq(name, "proc")) return 1;
    if (str_eq(name, "for")) return 1;
    if (str_eq(name, "foreach")) return 1;
    return 0;
}

/* ---- Block stack (global, avoids large stack locals) ---- */

#define BLOCK_STACK_SIZE 4096
char block_stack_pool[BLOCK_STACK_SIZE];
int  block_stack_top;

/* Push a string onto the block stack. Returns start index, or -1 on overflow. */
int block_push(char *s) {
    int start = block_stack_top;
    int i = 0;
    while (s[i] && block_stack_top < BLOCK_STACK_SIZE - 1) {
        block_stack_pool[block_stack_top] = s[i];
        block_stack_top = block_stack_top + 1;
        i = i + 1;
    }
    block_stack_pool[block_stack_top] = 0;
    block_stack_top = block_stack_top + 1;
    return start;
}

char *block_get(int idx) {
    return block_stack_pool + idx;
}

void block_pop(int pos) {
    block_stack_top = pos;
}

/* Global line buffer for eval_block (avoids 512-byte stack local) */
char eval_line_buf[MAX_LINE_LEN];

/* ---- Line and block evaluation ---- */

int eval_line(char *line) {
    int count = tokenize_line(line);
    if (count < 0) return RET_ERR;
    if (count == 0) return RET_OK;

    char *name = token_str(0);

    /* Reserved keyword check */
    int ci = cmd_find(name);
    if (ci < 0) {
        if (is_reserved(name)) {
            uart_puts("error: reserved keyword: ");
        } else {
            uart_puts("error: unknown command: ");
        }
        uart_puts(name);
        uart_newline();
        return RET_ERR;
    }

    /* Pre-expansion commands: exists? */
    if (str_eq(name, "exists?")) {
        int (*fn)() = cmd_handler[ci];
        fn();
        return RET_OK;
    }

    if (expand_variables() < 0) return RET_ERR;

    int (*fn)() = cmd_handler[ci];
    return fn();
}

int eval_block(char *block) {
    int blen = str_len(block);
    if (blen < 2 || block[0] != 123 || block[blen - 1] != 125) {
        uart_puts("error: invalid block");
        uart_newline();
        return RET_ERR;
    }

    /* Push block content onto block stack so it survives recursive calls */
    int save = block_stack_top;
    int bi = block_push(block);
    char *blk = block_get(bi);
    int i = 1; /* skip '{' */
    int end = blen - 1; /* before '}' */

    while (i < end) {
        /* Skip whitespace and newlines */
        while (i < end && (blk[i] == 32 || blk[i] == 9 || blk[i] == 10))
            i = i + 1;
        if (i >= end) break;

        /* Collect one line into global buffer, respecting brace nesting */
        int li = 0;
        int depth = 0;
        while (i < end && li < MAX_LINE_LEN - 1) {
            if (blk[i] == 123) depth = depth + 1;
            if (blk[i] == 125) depth = depth - 1;
            if ((blk[i] == 10 || blk[i] == 59) && depth == 0) {
                i = i + 1;
                break;
            }
            eval_line_buf[li] = blk[i];
            li = li + 1;
            i = i + 1;
        }
        eval_line_buf[li] = 0;
        if (li == 0) continue;

        int ret = eval_line(eval_line_buf);
        if (ret == RET_BREAK || ret == RET_CONTINUE || ret == RET_ERR) {
            block_pop(save);
            return ret;
        }
        if (exit_flag) { block_pop(save); return RET_OK; }
    }
    block_pop(save);
    return RET_OK;
}

/* ---- Comparison commands ---- */

int cmd_eq() {
    if (expand_count < 3) {
        uart_puts("error: eq: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result;
    if (is_int_str(a) && is_int_str(b)) {
        result = str_to_int(a) == str_to_int(b);
    } else {
        result = str_eq(a, b);
    }
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_ne() {
    if (expand_count < 3) {
        uart_puts("error: ne: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result;
    if (is_int_str(a) && is_int_str(b)) {
        result = str_to_int(a) != str_to_int(b);
    } else {
        if (str_eq(a, b)) {
            result = 0;
        } else {
            result = 1;
        }
    }
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_lt() {
    if (expand_count < 3) {
        uart_puts("error: lt: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result = 0;
    if (is_int_str(a) && is_int_str(b)) {
        if (str_to_int(a) < str_to_int(b)) result = 1;
    }
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_gt() {
    if (expand_count < 3) {
        uart_puts("error: gt: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result = 0;
    if (is_int_str(a) && is_int_str(b)) {
        if (str_to_int(a) > str_to_int(b)) result = 1;
    }
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_le() {
    if (expand_count < 3) {
        uart_puts("error: le: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result = 0;
    if (is_int_str(a) && is_int_str(b)) {
        if (str_to_int(a) <= str_to_int(b)) result = 1;
    }
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_ge() {
    if (expand_count < 3) {
        uart_puts("error: ge: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *a = exp_str(1);
    char *b = exp_str(2);
    int result = 0;
    if (is_int_str(a) && is_int_str(b)) {
        if (str_to_int(a) >= str_to_int(b)) result = 1;
    }
    last_result = val_new_int(result);
    return RET_OK;
}

/* ---- Logic commands ---- */

int cmd_and() {
    if (expand_count < 3) {
        uart_puts("error: and: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    int va = val_from_str(exp_str(1));
    int vb = val_from_str(exp_str(2));
    int result = 0;
    if (val_is_truthy(va) && val_is_truthy(vb)) result = 1;
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_or() {
    if (expand_count < 3) {
        uart_puts("error: or: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    int va = val_from_str(exp_str(1));
    int vb = val_from_str(exp_str(2));
    int result = 0;
    if (val_is_truthy(va) || val_is_truthy(vb)) result = 1;
    last_result = val_new_int(result);
    return RET_OK;
}

int cmd_not() {
    if (expand_count < 2) {
        uart_puts("error: not: expected 1 arg");
        uart_newline();
        return RET_ERR;
    }
    int va = val_from_str(exp_str(1));
    int result = 0;
    if (!val_is_truthy(va)) result = 1;
    last_result = val_new_int(result);
    return RET_OK;
}

/* ---- Control flow commands ---- */

int cmd_if() {
    if (expand_count < 3) {
        uart_puts("error: if: expected at least 2 args");
        uart_newline();
        return RET_ERR;
    }
    int save = block_stack_top;
    int ci = block_push(exp_str(1));
    int bi = block_push(exp_str(2));
    int ei = -1;
    int has_else = 0;
    if (expand_count >= 5 && str_eq(exp_str(3), "else")) {
        ei = block_push(exp_str(4));
        has_else = 1;
    }

    int ret = eval_block(block_get(ci));
    if (ret == RET_ERR) { block_pop(save); return RET_ERR; }

    if (val_is_truthy(last_result)) {
        ret = eval_block(block_get(bi));
    } else if (has_else) {
        ret = eval_block(block_get(ei));
    } else {
        ret = RET_OK;
    }
    block_pop(save);
    return ret;
}

int cmd_while() {
    if (expand_count < 3) {
        uart_puts("error: while: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    int save = block_stack_top;
    int ci = block_push(exp_str(1));
    int bi = block_push(exp_str(2));

    int max_iter = 10000;
    while (max_iter > 0) {
        max_iter = max_iter - 1;

        int ret = eval_block(block_get(ci));
        if (ret == RET_ERR) { block_pop(save); return RET_ERR; }
        if (!val_is_truthy(last_result)) break;

        ret = eval_block(block_get(bi));
        if (ret == RET_BREAK) break;
        if (ret == RET_ERR) { block_pop(save); return RET_ERR; }
        if (exit_flag) break;
        /* RET_CONTINUE: just continue the loop */
    }
    block_pop(save);
    return RET_OK;
}

int cmd_break() {
    return RET_BREAK;
}

int cmd_continue_() {
    return RET_CONTINUE;
}

/* ---- Pragma command ---- */

int cmd_pragma() {
    if (expand_count != 3) {
        uart_puts("error: pragma: expected 2 args");
        uart_newline();
        return RET_ERR;
    }
    char *key = exp_str(1);
    char *val_s = exp_str(2);
    if (str_eq(key, "run-rc") && str_eq(val_s, "on")) {
        pragma_run_rc = 1;
        return RET_OK;
    }
    uart_puts("error: unknown pragma: ");
    uart_puts(key);
    uart_putc(32);
    uart_puts(val_s);
    uart_newline();
    return RET_ERR;
}

/* ---- Run command ---- */

/* Build a structured $rc record and store it in the "rc" variable.
 * run_code: 0=ok, 1=not-found, 2=crash, 3=exec-failure, 4=usage-error
 * detail: prog exit code (run_code=0) or error code (others)
 * msg: human-readable message (NULL for success case)
 * kind: category string */
int set_rc_record(int run_code, int detail, char *msg, char *kind) {
    int rec = val_new_rec();
    if (rec < 0) return -1;

    /* $rc.run */
    int fn_run = val_new_str("run");
    int fv_run = val_new_int(run_code);
    val_rec_set(rec, fn_run, fv_run);

    /* $rc.kind */
    int fn_kind = val_new_str("kind");
    int fv_kind = val_new_str(kind);
    val_rec_set(rec, fn_kind, fv_kind);

    if (run_code == 0) {
        /* Success: $rc.prog = detail */
        int fn_prog = val_new_str("prog");
        int fv_prog = val_new_int(detail);
        val_rec_set(rec, fn_prog, fv_prog);
    } else {
        /* Failure: $rc.err and $rc.msg */
        int fn_err = val_new_str("err");
        int fv_err = val_new_int(detail);
        val_rec_set(rec, fn_err, fv_err);
        int fn_msg = val_new_str("msg");
        int fv_msg = val_new_str(msg);
        val_rec_set(rec, fn_msg, fv_msg);
    }

    env_set("rc", rec);
    rc_defined = 1;
    return 0;
}

int cmd_run() {
    if (!pragma_run_rc) {
        /* Without pragma, run still works but doesn't set $rc */
    }

    if (expand_count < 2) {
        /* Usage error */
        if (pragma_run_rc) {
            set_rc_record(4, 0, "invalid run invocation", "usage-error");
        }
        uart_puts("error: run: expected program name");
        uart_newline();
        return RET_ERR;
    }

    /* For now, on COR24, process execution is not yet available.
     * Simulate a "not found" result for any program.
     * Future: use OS syscall to launch child binary. */

    if (pragma_run_rc) {
        set_rc_record(1, 1, "binary not found", "not-found");
    }

    /* The run command evaluates to $rc.run */
    last_result = val_new_int(1); /* not-found */
    return RET_OK;
}

/* ---- Arithmetic helper: incr ---- */

int cmd_incr() {
    if (expand_count < 2) {
        uart_puts("error: incr: expected variable name");
        uart_newline();
        return RET_ERR;
    }
    char *name = exp_str(1);
    int amount = 1;
    if (expand_count >= 3) {
        amount = str_to_int(exp_str(2));
    }
    int vi = env_get(name);
    if (vi < 0) {
        uart_puts("error: incr: undefined variable: ");
        uart_puts(name);
        uart_newline();
        return RET_ERR;
    }
    if (val_type[vi] != VAL_INT) {
        uart_puts("error: incr: not an integer: ");
        uart_puts(name);
        uart_newline();
        return RET_ERR;
    }
    val_int[vi] = val_int[vi] + amount;
    last_result = vi;
    return RET_OK;
}

/* ---- Command registration ---- */

void register_builtins() {
    cmd_count = 0;
    cmd_register("echo", cmd_echo);
    cmd_register("set", cmd_set);
    cmd_register("exit", cmd_exit);
    cmd_register("exists?", cmd_exists);
    /* Comparison commands */
    cmd_register("eq", cmd_eq);
    cmd_register("ne", cmd_ne);
    cmd_register("lt", cmd_lt);
    cmd_register("gt", cmd_gt);
    cmd_register("le", cmd_le);
    cmd_register("ge", cmd_ge);
    /* Logic commands */
    cmd_register("and", cmd_and);
    cmd_register("or", cmd_or);
    cmd_register("not", cmd_not);
    /* Control flow */
    cmd_register("if", cmd_if);
    cmd_register("while", cmd_while);
    cmd_register("break", cmd_break);
    cmd_register("continue", cmd_continue_);
    cmd_register("incr", cmd_incr);
    /* Pragma and run */
    cmd_register("pragma", cmd_pragma);
    cmd_register("run", cmd_run);
    /* Debug commands */
    cmd_register("_valtest", cmd_valtest);
    cmd_register("_toktest", cmd_toktest);
}

/* ---- Main ---- */

int main() {
    uart_puts("sws 0.1");
    uart_newline();

    val_reset();
    env_reset();
    exit_flag = 0;
    exit_code = 0;
    pragma_run_rc = 0;
    rc_defined = 0;
    block_stack_top = 0;
    register_builtins();

    last_result = val_new_int(0);

    while (!exit_flag) {
        uart_puts("sws> ");
        int len = read_line(line_buf, MAX_LINE_LEN);
        if (len < 0) break; /* EOF */
        if (len == 0) continue; /* empty line */

        eval_line(line_buf);
    }

    halt();
    return 0;
}
