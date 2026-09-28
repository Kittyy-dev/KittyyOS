#include <string.h>
#include <stdint.h>
#include <pmm.h>
#include <colors.h>
#include <kprint.h>

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
    while (*d) d++;        // ans Ende gehen
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
    size_t len = strlen(s) + 1;   

    // Eine Page holen (physische Adresse)
    uint64_t phys = pmm_alloc_page();
    if (!phys) return NULL;

    // phys == virt (Identity Mapping)
    char* out = (char*)phys;

    // String kopieren
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