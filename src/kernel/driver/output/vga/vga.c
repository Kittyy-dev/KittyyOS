#include <stdint.h>
#include <stdarg.h>
#include <vga.h>
#include <ports.h>
#include <hex_colors.h>
#include <colors.h>
#include <flush.h>
#include <kprint.h>
#include <paging.h>

#define VGA_MEM ((uint16_t*)(HHDM_BASE + 0xB8000))

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
    uint8_t attr = (bg << 4) | VGA_BLACK; // FG
    volatile uint16_t* vga = VGA_MEM;
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
    volatile uint16_t* vga = VGA_MEM;
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
    volatile uint16_t* vga = VGA_MEM;
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

void clear_screen(void) {
    volatile uint16_t* vga = VGA_MEM;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = vga_entry(' ', DEFAULT_COLOR);
    }
    cursor_x = 0;
    cursor_y = 0;
    move_cursor(cursor_x, cursor_y);
}

void kernel_info(const char* info) {
    volatile uint16_t* vga = VGA_MEM;
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