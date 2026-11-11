#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <pmm.h>
#include <colors.h>
#include <kprint.h>
#include <stdarg.h>
#include <paging.h>

void itoa(int value, char* str, int base) {
    char* rc = str;
    char* ptr;
    char* low;

    if(value == 0) {
        *str++ = '0';
        *str = '\0';
        return;
    }

    int sign = value;

    if(sign < 0 && base == 10) {
        value = -value;
    }

    while(value !=0) {
        int rem = value % base;
        *str++ = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value /= base;
    }

    if (sign < 0 && base == 10) {
        *str++ = '-';
    }

    *str = '\0';

    low = rc;
    ptr = str - 1;

    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
}

void u64toa(uint64_t value, char *str, int base) {
    char *rc = str;
    char *ptr;
    char *low;

    if (value == 0) {
        *str++ = '0';
        *str = 0;
        return;
    }

    while (value) {
        uint64_t rem = value % base;
        *str++ = (rem > 9) ? ('a' + rem - 10) : ('0' + rem);
        value /= base;
    }

    *str = 0;

    low = rc;
    ptr = str - 1;

    while (low < ptr) {
        char t = *low;
        *low++ = *ptr;
        *ptr-- = t;
    }
}

void ftoa(double value, char* buffer, int precision) {
    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }

    uint64_t int_part = (uint64_t)value;
    double frac_part = value - (double)int_part;

    char intbuf[32];
    u64toa(int_part, intbuf, 10);

    int i = 0;

    while (intbuf[i]) {
        *buffer++ = intbuf[i++];
    }

    *buffer++ = '.';

    for (int p = 0; p < precision; p++) {
        frac_part *= 10.0;
        int digit = (int)frac_part;
        *buffer++ = '0' + digit;
        frac_part -= digit;
    }

    *buffer = 0;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* a = s1;
    const unsigned char* b = s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void* memset(void* dest, int c, size_t n) {
    unsigned char* d = dest;
    for (size_t i = 0; i < n; i++) d[i] = (unsigned char)c;
    return dest;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

int strncmp(const char *a, const char *b, int n) {
    while (n-- > 0) {
        if (*a != *b || *a == 0 || *b == 0)
            return *(const unsigned char*)a - *(const unsigned char*)b;
        a++;
        b++;
    }
    return 0;
}

int strlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

int strcasecmp(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

char* strdup(const char* s) {
    if (!s) return NULL;

    size_t len = strlen(s) + 1;

    uint64_t phys = pmm_alloc_page();
    if (!phys) return NULL;

    char* out = (char*)(HHDM_BASE + phys);

    memcpy(out, s, len);

    return out;
}


char* strtok(char* str, const char* delimiters) {
    static char* next = NULL;

    if (str != NULL) {
        next = str;
    }

    if (next == NULL) {
        return NULL;
    }

    while (*next) {
        const char* d = delimiters;

        while (*d) {
            if (*next == *d)
                break;
            d++;
        }

        if (*d == '\0')
            break;

        next++;
    }

    if (*next == '\0') {
        next = NULL;
        return NULL;
    }

    char* token = next;

    while (*next) {
        const char* d = delimiters;

        while (*d) {
            if (*next == *d)
                break;
            d++;
        }

        if (*d != '\0') {
            *next = '\0';
            next++;
            return token;
        }

        next++;
    }

    next = NULL;
    return token;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;

    if (!dest || !src || n == 0)
        return dest;

    for (i = 0; i < n - 1 && src[i] != '\0'; i++)
        dest[i] = src[i];

    dest[i] = '\0';

    return dest;
}

static void append_char(char *out, size_t *i, char c) {
    if (*i < BUF_SIZE - 1) {
        out[(*i)++] = c;
    }
}

static void append_number(char *out, size_t *i, uint64_t value, unsigned base, int width, int zero_pad, int negative, int uppercase) {
    char buffer[65];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    int n = 0;

    do {
        buffer[n++] = digits[value % base];
        value /= base;
    } while (value && n < (int)sizeof(buffer));

    int total = n + negative;
    char pad = zero_pad ? '0' : ' ';

    while (total < width && *i < BUF_SIZE - 1) {
        append_char(out, i, pad);
        width--;
    }

    if (negative) {
        append_char(out, i, '-');
    }

    while (n > 0 && *i < BUF_SIZE - 1) {
        append_char(out, i, buffer[--n]);
    }
}

void append_string(char *out, size_t *i, const char *s) {
    if (!s) {
        s = "(null)";
    }

    while (*s && *i < BUF_SIZE - 1) {
        out[(*i)++] = *s++;
    }
}

char *fmt_func(const char *fmt, va_list args) {
    static char out[BUF_SIZE];
    size_t i = 0;

    while (*fmt && i < BUF_SIZE - 1) {
        if (*fmt != '%') {
            append_char(out, &i, *fmt++);
            continue;
        }

        fmt++;

        if (*fmt == '%') {
            append_char(out, &i, '%');
            fmt++;
            continue;
        }

        int zero_pad = 0;
        int width = 0;
        int longlong = 0;
        int long_mod = 0;
        int short_mod = 0;
        int char_mod = 0;
        int size_mod = 0;

        if (*fmt == '0') {
            zero_pad = 1;
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            if (width < 100) {
                width = width * 10 + (*fmt - '0');
            }
            fmt++;
        }

        if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') {
                char_mod = 1;
                fmt++;
            } else {
                short_mod = 1;
            }
        } else if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                longlong = 1;
                fmt++;
            } else {
                long_mod = 1;
            }
        } else if (*fmt == 'z') {
            size_mod = 1;
            fmt++;
        }

        switch (*fmt) {
        case 'd':
        case 'i': {
            int64_t val;

            if (longlong) {
                val = va_arg(args, long long);
            } else if (long_mod) {
                val = va_arg(args, long);
            } else if (size_mod) {
                val = (int64_t)va_arg(args, ptrdiff_t);
            } else {
                val = va_arg(args, int);
                if (short_mod) val = (short)val;
                if (char_mod) val = (signed char)val;
            }

            int neg = val < 0;
            uint64_t magnitude = neg
                ? (uint64_t)(-(val + 1)) + 1
                : (uint64_t)val;

            append_number(out, &i, magnitude, 10, width, zero_pad, neg, 0);
            break;
        }

        case 'u':
        case 'x':
        case 'X': {
            uint64_t val;

            if (longlong) {
                val = va_arg(args, unsigned long long);
            } else if (long_mod) {
                val = va_arg(args, unsigned long);
            } else if (size_mod) {
                val = va_arg(args, size_t);
            } else {
                val = va_arg(args, unsigned int);
                if (short_mod) val = (unsigned short)val;
                if (char_mod) val = (unsigned char)val;
            }

            unsigned base = (*fmt == 'x' || *fmt == 'X') ? 16 : 10;

            append_number(out, &i, val, base,
                          width, zero_pad, 0, *fmt == 'X');
            break;
        }

        case 'p': {
            uintptr_t ptr = (uintptr_t)va_arg(args, void *);
            append_string(out, &i, "0x");
            append_number(out, &i, ptr, 16,
                          (int)(sizeof(void *) * 2), 1, 0, 0);
            break;
        }

        case 's':
            append_string(out, &i, va_arg(args, const char *));
            break;

        case 'c':
            append_char(out, &i, (char)va_arg(args, int));
            break;

        default:
            append_char(out, &i, '%');
            if (*fmt) append_char(out, &i, *fmt);
            break;
        }

        if (*fmt) fmt++;
    }

    out[i] = '\0';

    return out;
}