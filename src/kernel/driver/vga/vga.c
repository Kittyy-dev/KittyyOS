#include <stdint.h>
#include <stdarg.h>
#include <vga.h>
#include <ports.h>
#include <hex_colors.h>
#include <colors.h>
#include <flush.h>
#include <kprint.h>
#include <string.h>

int cursor_x = 0;
int cursor_y = 0;

uint8_t vga_fg;
uint8_t vga_bg;

uint8_t DEFAULT_COLOR = 0x0F;

uint8_t color_locked = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint8_t)c;
}

void vga_put_color_space(uint8_t bg) {
    uint8_t attr = (bg << 4) | VGA_BLACK; // FG egal
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    vga[cursor_y * 80 + cursor_x] = (attr << 8) | ' ';
    cursor_x++;
}

void vga_set_fg(uint8_t fg) {
    vga_fg = fg & 0x0F;
    DEFAULT_COLOR = (vga_bg << 4) | vga_fg;
    color_locked = 1;
}

void vga_set_bg(uint8_t bg) {
    vga_bg = bg & 0x0F;
    DEFAULT_COLOR = (vga_bg << 4) | vga_fg;
    color_locked = 1;
}

void reset() {
    DEFAULT_COLOR = 0x0F;
    color_locked = 0;
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

uint8_t vga_color_from_kcolor(KColor color) {
    if (color >= 16) {
        return vga_colors[0x07];
    }

    return vga_colors[color];
}

void vga_print(const char* str) {
    while (*str) {
        uint8_t code = (uint8_t)*str;

        // Dark colors: 0x01 - 0x08
        if (code >= 0x01 && code <= 0x08) {
            DEFAULT_COLOR = vga_colors[code - 1];
            str++;
            continue;
        }

        // Bright colors: 0x11 - 0x18
        if (code >= 0x11 && code <= 0x18) {
            DEFAULT_COLOR = vga_colors[code - 0x11 + 8];
            str++;
            continue;
        }

        vga_putc(*str++);
    }
}

void vga_printf(const char* fmt, va_list args) {

    char out[4096];

    int i = 0;

    while (*fmt && i < 255) {
        if (*fmt == '%') {
            fmt++;

            char buffer[64];

            int zero_pad = 0;
            int width = 0;
            int longlong = 0;

            if (*fmt == '0') {
                zero_pad = 1;
                fmt++;
            }

            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            if (*fmt == 'l' && *(fmt + 1) == 'l') {
                longlong = 1;
                fmt += 2;
            }

            switch (*fmt) {
                case 'd': {
                    itoa(va_arg(args, int), buffer, 10);

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }
                    break;
                }

                case 'x': {
                    if (longlong) {
                        u64toa(va_arg(args, uint64_t), buffer, 16);
                    } else {
                        itoa(va_arg(args, int), buffer, 16);
                    }

                    int len = 0;

                    while (buffer[len]) {
                        len++;
                    }

                    while (zero_pad && len < width) {
                        out[i++] = '0';
                        width--;
                    }

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

                case 'p' : {
                    uintptr_t ptr = (uintptr_t)va_arg(args, void*);

                    out[i++] = '0';
                    out[i++] = 'x';

                    #if UINTPTR_MAX == UINT64_MAX
                        u64toa((uintptr_t)ptr, buffer, 16);
                    #else 
                        itoa((int)ptr, buffer, 16);
                    #endif

                    int len = 0;

                    while (buffer[len]) {
                        len++;
                    }

                    int ptr_width = sizeof(void*) * 2;

                    while (len < ptr_width) {
                        out[i++] = '0';
                        len++;
                    }

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

                case 's' : {
                    char* s = va_arg(args, char*);

                    for (int j = 0; s[j]; j++) {
                        out[i++] = s[j];
                    }
                    break;
                }

                case 'c': {
                    out[i++] = (char)va_arg(args, int);
                    break;
                }

                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    u64toa((uint64_t)val, buffer, 10);

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

                case 'f': {
                    double val = va_arg(args, double);
                    ftoa(val, buffer, 3);

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

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

    // va_end(args);

    vga_print(out);
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

void vga_print_at(int x, int y, const char *text, uint8_t color) {
    if (!text) {
        return;
    }

    while (*text) {
        if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
            return;
        }

        if (*text == '\n') {
            x = 0;
            y++;
            text++;
            continue;
        }

        putchar_at(x, y, *text, color);

        x++;
        text++;
    }
}