#include "vbe.h"
#include <font.h>
#include <tty.h>
#include <stdint.h>
#include <devfs.h>
#include <kprint.h>
#include <keyboard.h>
#include <stdbool.h>
#include <stdlib.h>
#include <colors.h>

static tty_t ttys[TTY_COUNT];
static int current_tty = 0;

static int tty_cols;
static int tty_rows;

void tty_init(void) {
    tty_cols = width() / CHAR_WIDTH;
    tty_rows = height() / CHAR_HEIGHT;

    for (int i = 0; i < TTY_COUNT; i++) {
        ttys[i].x = 0;
        ttys[i].y = 0;
        ttys[i].head = 0;
        ttys[i].tail = 0;
        ttys[i].active = false;

        ttys[i].screen = malloc(tty_cols * tty_rows * sizeof(tty_cell_t));

        if (!ttys[i].screen) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "Failed to allocate screen buffer for tty!\n");
            return;
        }

        for (int j = 0; j < tty_cols * tty_rows; j++) {
            ttys[i].screen[j].c = ' ';
            ttys[i].screen[j].color = VBE_DEFAULT_FG;
        }
    }

    ttys[0].active = true;

    devfs_register_device("tty0", tty_read, tty_write, &ttys[0]);
    devfs_register_device("tty1", tty_read, tty_write, &ttys[1]);
    devfs_register_device("tty2", tty_read, tty_write, &ttys[2]);
    devfs_register_device("tty3", tty_read, tty_write, &ttys[3]);
}

static void tty_put_cell(tty_t *tty, int x, int y, char c, uint32_t color) {
    if (!tty || !tty->screen) {
        return;
    }

    if (x < 0 || x >= tty_cols) {
        return;
    }

    if (y < 0 || y >= tty_rows) {
        return;
    }

    tty->screen[y * tty_cols + x].c = c;
    tty->screen[y * tty_cols + x].color = color;
}

static void tty_redraw(tty_t *tty) {
    if (!tty || !tty->screen) {
        return;
    }

    for (int y = 0; y < tty_rows; y++) {
        for (int x = 0; x < tty_cols; x++) {
            tty_cell_t *cell = &tty->screen[y * tty_cols + x];

            kputchar_at(x * CHAR_WIDTH, y * CHAR_HEIGHT, cell->c, cell->color);
        }
    }
}

static void tty_putc(tty_t *tty, char c) {
    if (!tty) {
        return;
    }

    if (c == '\n') {
        tty->x = 0;
        tty->y++;

        if (tty->y >= tty_rows) {
            tty->y = tty_rows - 1;
        }

        return;
    }

    tty_put_cell(tty, tty->x, tty->y, c, VBE_DEFAULT_FG);

    if (tty->active) {
        kputchar_at(tty->x * CHAR_WIDTH, tty->y * CHAR_HEIGHT, c, VBE_DEFAULT_FG);
    }

    tty->x++;

    if (tty->x >= tty_cols) {
        tty->x = 0;
        tty->y++;

        if (tty->y >= tty_rows) {
            tty->y = tty_rows - 1;
        }
    }
}

tty_t *tty_get(int id) {
    if (id < 0 || id >= TTY_COUNT) {
        return NULL;
    }

    return &ttys[id];
}

tty_t *tty_current(void) {
    return &ttys[current_tty];
}

void tty_switch(int id) {
    if (id < 0 || id >= TTY_COUNT) {
        return;
    }

    if (id == current_tty) {
        return;
    }

    ttys[current_tty].active = false;

    current_tty = id;
    ttys[current_tty].active = true;

    tty_redraw(&ttys[current_tty]);
}

size_t tty_read(vfs_node_t *node, void *buf, size_t len) {
    if (!node || !buf || len == 0) {
        return 0;
    }

    devfs_device_t *device = (devfs_device_t *)node->internal;

    if (!device || !device->private_data) {
        return 0;
    }

    tty_t *tty = (tty_t *)device->private_data;

    char *out = (char *)buf;
    size_t count = 0;

    while (count < len) {
        if (tty->head == tty->tail) {
            break;
        }

        out[count++] = tty->input[tty->tail];

        tty->tail = (tty->tail + 1) % TTY_BUF_SIZE;
    }

    return count;
}

size_t tty_write(vfs_node_t *node, const void *buf, size_t len) {
    if (!node || !buf || len == 0) {
        return 0;
    }

    devfs_device_t *device = (devfs_device_t *)node->internal;

    if (!device || !device->private_data) {
        return 0;
    }

    tty_t *tty = (tty_t *)device->private_data;

    const char *data = (const char *)buf;

    for (size_t i = 0; i < len; i++) {
        tty_putc(tty, data[i]);
    }

    return len;
}