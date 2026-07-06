#include <stdint.h>
#include <string.h>

int kstrcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int kstrlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void kstrcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}