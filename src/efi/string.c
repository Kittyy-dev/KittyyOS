#include <string.h>
#include <stdint.h>
#include <pmm.h>
#include <colors.h>
#include <kprint.h>

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