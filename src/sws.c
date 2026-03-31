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

#define MAX_LINE_LEN   4096
#define MAX_TOKENS      256
#define MAX_TOKEN_LEN   256
#define MAX_VARS        256
#define MAX_VAR_NAME     64
#define MAX_VAR_VAL     256
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

int main() {
    uart_puts("sws 0.1");
    uart_newline();
    halt();
    return 0;
}
