#include <tty.h>
#include <keyboard.h>
#include <ports.h>
#include <vga.h>
#include <kprint.h>
#include <vfs.h>
#include <stdbool.h>
#include <stdint.h>

#define KBD_BUF_SIZE 128

extern int kcursor_x;
extern int kcursor_y;

static bool shift = false;
static bool extended = false;

static bool ctrl = false;
static bool alt = false;

static char kbd_buf[KBD_BUF_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

bool keyboard_enabled = false;
int ignore_first_enter = 0;

#define KEY_UP     ((char)0xF1)
#define KEY_DOWN   ((char)0xF2)
#define KEY_LEFT   ((char)0xF3)
#define KEY_RIGHT  ((char)0xF4)

static const unsigned char layout[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0',
    [0x0C] = 0xE1,
    [0x0D] = '\'',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'z', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p',
    [0x1A] = 0x81,
    [0x1B] = '+',
    [0x1C] = '\n',

    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = 0x94,
    [0x28] = 0x84,
    [0x29] = '#',

    [0x2B] = '<',
    [0x2C] = 'y', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',', [0x34] = '.', [0x35] = '-',

    [0x39] = ' '
};

static const unsigned char layout_shift[128] = {
    [0x02] = '!', [0x03] = '"', [0x04] = 0x15,
    [0x05] = '$', [0x06] = '%', [0x07] = '&',
    [0x08] = '/', [0x09] = '(',
    [0x0A] = ')', [0x0B] = '=', [0x0C] = '?',
    [0x0D] = '`',

    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R',
    [0x14] = 'T', [0x15] = 'Z', [0x16] = 'U', [0x17] = 'I',
    [0x18] = 'O', [0x19] = 'P',
    [0x1A] = 0x9A,
    [0x1B] = '*',
    [0x1C] = '\n',

    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F',
    [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = 0x99,
    [0x28] = 0x8E,
    [0x29] = '\'',

    [0x2B] = '>',
    [0x2C] = 'Y', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V',
    [0x30] = 'B', [0x31] = 'N', [0x32] = 'M',
    [0x33] = ';', [0x34] = ':', [0x35] = '_',

    [0x39] = ' '
};

static void kbd_push(char c) {
    int next = (kbd_head + 1) % KBD_BUF_SIZE;

    if (next != kbd_tail) {
        kbd_buf[kbd_head] = c;
        kbd_head = next;
    }
}

static char kbd_pop(void) {
    while (kbd_head == kbd_tail);

    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;

    return c;
}

void keyboard_handle_scancode(uint8_t sc) {
    if (sc == 0x2A || sc == 0x36) {
        shift = true;
        return;
    }

    if (sc == 0xAA || sc == 0xB6) {
        shift = false;
        return;
    }

    if (sc == 0x1D) {
        ctrl = true;
        return;
    }

    if (sc == 0x9D) {
        ctrl = false;
        return;
    }

    if (sc == 0x38) {
        alt = true;
        return;
    }

    if (sc == 0xB8) {
        alt = false;
        return;
    }

    if (ctrl && alt && sc == 0x3B) {
        tty_switch(0);
        return;
    }

    if (ctrl && alt && sc == 0x3C) {
        tty_switch(1);
        return;
    }

    if (sc >= 128) {
        return;
    }

    char c = shift ? layout_shift[sc] : layout[sc];

    if (ignore_first_enter && (c == '\n' || c == '\r')) {
        ignore_first_enter = 0;
        return;
    }

    if (c) {
        kbd_push(c);
    }
}

static void handle_extended_key(uint8_t sc) {
    switch (sc) {
        case 0x48:
            kbd_push(KEY_UP);
            break;

        case 0x50:
            kbd_push(KEY_DOWN);
            break;

        case 0x4B:
            kbd_push(KEY_LEFT);
            break;

        case 0x4D:
            kbd_push(KEY_RIGHT);
            break;

        default:
            break;
    }
}

char get_char_from_keyboard(void) {
    return kbd_pop();
}

void flush_keyboard_buffer(void) {
    while (inb(0x64) & 1) {
        inb(0x60);
    }
}

void shell_readline(char *out, int max) {
    int len = 0;

    int x = get_cursor_x();
    int y = get_cursor_y();

    while (1) {

        char c = get_char_from_keyboard();

        if (c == '\n' || c == '\r') {

            out[len] = '\0';

            x = 0;
            y++;

            if (y >= height()) {
                kscroll_screen();
                y = height() - 1;
            }

            kcursor(x, y);

            return;
        }
        
        if (c == KEY_LEFT) {
            continue;
        }

        if (c == KEY_RIGHT) {
            continue;
        }

        if (c == KEY_UP) {
            continue;
        }
        
        if (c == KEY_DOWN) {
            continue;
        }

        if (c == '\b') {
            if (len > 0) {
                len--;
                out[len] = '\0';

                x--;

                kputchar_at(x, y, ' ', DEFAULT_COLOR);

                kcursor(x, y);
            }

            continue;
        }

        if (c >= 32 && len < max - 1) {

            out[len++] = c;
            out[len] = '\0';

            kputchar_at(x, y, c, DEFAULT_COLOR);

            x++;


            if (x >= width()) {
                x = 0;
                y++;

                if (y >= height()) {
                    kscroll_screen();
                    y = height() - 1;
                }
            }

            kcursor(x, y);
        }
    }
}

bool keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}

char get_char_from_keyboard_nonblocking(void) {
    if (kbd_head == kbd_tail) {
        return 0;
    }

    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;

    return c;
}

void keyboard_handler(void) {
    if (!keyboard_enabled)
        return;

    uint8_t sc = inb(0x60);

    if (sc == 0xE0) {
        extended = true;
        return;
    }

    if (extended) {
        handle_extended_key(sc);
        extended = false;
        return;
    }

    keyboard_handle_scancode(sc);
}
