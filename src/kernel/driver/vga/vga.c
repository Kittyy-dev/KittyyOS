#include <stdint.h>
#include <stdarg.h>
#include <vga.h>
#include <ports.h>
#include <hex_colors.h>
#include <colors.h>
#include <flush.h>

int cursor_x = 0;
int cursor_y = 0;

uint8_t DEFAULT_COLOR = 0x0F;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint8_t)c;
}

void putchar_at(int x, int y, char c, uint8_t color) {
    volatile uint16_t* vga = (uint16_t*) 0xB8000;
    int index = y * VGA_WIDTH + x;
    vga[index] = vga_entry(c, color);
}

void move_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void scroll_screen(void) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            int from = y * VGA_WIDTH + x;
            int to = (y - 1) * VGA_WIDTH + x;
            vga[to] = vga[from];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        int index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga[index] = vga_entry(' ', 0x0F);
    }
    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
    move_cursor(cursor_x, cursor_y);
}

void vga_putc(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        putchar_at(cursor_x, cursor_y, c, DEFAULT_COLOR);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
    }

    move_cursor(cursor_x, cursor_y);
}


void vga_print(const char* str) {
    DEFAULT_COLOR = 0x0F;
    // vga_putc('#');
    while (*str) {

        // ANSI COLOR PARSER
        if ((uint8_t)*str == 0x1B &&
            *(str + 1) == '[' &&
            (*(str + 2) == '0' || *(str + 2) == '1') &&
            *(str + 3) == ';' &&
            *(str + 4) == '3') {

            int bright = (*(str + 2) == '1');

            switch (*(str + 5)) {

                case '0':
                    DEFAULT_COLOR = bright ? VGA_DARK_GRAY : VGA_BLACK;
                    break;

                case '1':
                    DEFAULT_COLOR = bright ? VGA_LIGHT_RED : VGA_RED;
                    break;

                case '2':
                    DEFAULT_COLOR = bright ? VGA_LIGHT_GREEN : VGA_GREEN;
                    break;

                case '3':
                    DEFAULT_COLOR = bright ? VGA_YELLOW : VGA_BROWN;
                    break;

                case '4':
                    DEFAULT_COLOR = bright ? VGA_LIGHT_BLUE : VGA_BLUE;
                    break;

                case '5':
                    DEFAULT_COLOR = bright ? VGA_LIGHT_MAGENTA : VGA_MAGENTA;
                    break;

                case '6':
                    DEFAULT_COLOR = bright ? VGA_LIGHT_CYAN : VGA_CYAN;
                    break;

                case '7':
                    DEFAULT_COLOR = bright ? VGA_WHITE : VGA_LIGHT_GRAY;
                    break;
            }

            str += 7;
            // if (!*str) return;

        } // else (DEFAULT_COLOR = 0x0F);

        // NEWLINE
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;

            if (cursor_y >= VGA_HEIGHT) {
                scroll_screen();
            }

            move_cursor(cursor_x, cursor_y);
            str++;
            continue;
        }

        // NORMAL CHARACTER
        vga_putc(*str);
        str++;
    }
}

void clear_screen(void) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = vga_entry(' ', DEFAULT_COLOR);
    }
    cursor_x = 0;
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);
}

void kernel_info(const char* info) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    int i = 0;
    while (*info && i < VGA_WIDTH * VGA_HEIGHT) {
        vga[i++] = vga_entry(*info++, 0x0F);
    }
}

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

void vga_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char out[256];
    int i = 0;

    while (*fmt && i < 255) {
        if (*fmt == '%') {
            fmt++;

            char buffer[64];

            switch (*fmt) {
                case 'd': {
                    itoa(va_arg(args, int), buffer, 10);
                    for (int j = 0; buffer[j]; j++)
                        out[i++] = buffer[j];
                    break;
                }

                case 'x': {
                    itoa(va_arg(args, int), buffer, 16);
                    for (int j = 0; buffer[j]; j++)
                        out[i++] = buffer[j];
                    break;
                }

                case 's': {
                    char* s = va_arg(args, char*);
                    for (int j = 0; s[j]; j++)
                        out[i++] = s[j];
                    break;
                }

                case 'c':
                    out[i++] = (char)va_arg(args, int);
                    break;

                default:
                    out[i++] = '%';
                    out[i++] = *fmt;
                    break;
            }

        } else {
            out[i++] = *fmt;
        }

        fmt++;
    }

    out[i] = 0;

    va_end(args);

    vga_print(out);

    // flush(out, sizeof(out));
}

/*
void flush_vga() {
    flush(out, sizeof(out));
}
*/