#include <stdint.h>
#include <flush.h>

void flush(char* buf, int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = '\0';
    }
}