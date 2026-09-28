#ifndef TTY_H
#define TTY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <vfs.h>

#define TTY_COUNT 4
#define TTY_BUF_SIZE 128

typedef struct tty_cell {
    char c;
    uint32_t color;
} tty_cell_t;

typedef struct tty {
    int x;
    int y;

    tty_cell_t *screen;
    char input[TTY_BUF_SIZE];

    volatile int head;
    volatile int tail;

    bool active;
} tty_t;

void tty_init(void);
tty_t *tty_get(int id);
tty_t *tty_current(void);

void tty_switch(int id);

size_t tty_read(vfs_node_t *node, void *buf, size_t len);
size_t tty_write(vfs_node_t *node, const void *buf, size_t len);

#endif