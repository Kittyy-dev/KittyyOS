#include <stddef.h>
#include <stdint.h>
#include <syscalls.h>
#include <kprint.h>

long sys_write(int fd, const char *buf, size_t count) {
    if (fd != 1) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        kputc(buf[i]);
    }

    return count;
}