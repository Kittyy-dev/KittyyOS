#include <stdint.h>

#include <stdint.h>

typedef enum {
    KCOLOR_BLACK = 0,
    KCOLOR_RED,
    KCOLOR_GREEN,
    KCOLOR_YELLOW,
    KCOLOR_BLUE,
    KCOLOR_MAGENTA,
    KCOLOR_CYAN,
    KCOLOR_WHITE,

    KCOLOR_BRIGHT_BLACK,
    KCOLOR_BRIGHT_RED,
    KCOLOR_BRIGHT_GREEN,
    KCOLOR_BRIGHT_YELLOW,
    KCOLOR_BRIGHT_BLUE,
    KCOLOR_BRIGHT_MAGENTA,
    KCOLOR_BRIGHT_CYAN,
    KCOLOR_BRIGHT_WHITE
} KColor;

static const uint32_t vbe_colors[] = {
    0x000000, // black
    0xAA0000, // red
    0x00AA00, // green
    0xAA5500, // yellow
    0x0000AA, // blue
    0xAA00AA, // magenta
    0x00AAAA, // cyan
    0xAAAAAA, // white

    0x555555, // bright black
    0xFF5555, // bright red
    0x55FF55, // bright green
    0xFFFF55, // bright yellow
    0x5555FF, // bright blue
    0xFF55FF, // bright magenta
    0x55FFFF, // bright cyan
    0xFFFFFF  // bright white
};

static const uint8_t vga_colors[] = {
    0x00, // black
    0x04, // red
    0x02, // green
    0x06, // yellow / brown
    0x01, // blue
    0x05, // magenta
    0x03, // cyan
    0x07, // white

    0x08, // bright black
    0x0C, // bright red
    0x0A, // bright green
    0x0E, // bright yellow
    0x09, // bright blue
    0x0D, // bright magenta
    0x0B, // bright cyan
    0x0F  // bright white
};

uint8_t vga_color_from_kcolor(KColor color);
uint32_t vbe_color_from_kcolor(KColor color);

void check_video_mode(void);
void kputchar_at(int x, int y, char c, uint32_t color);
void kputc(char c);
void kprintf(const char* fmt, ...);
void kcursor(int x, int y);
void initcursor();
int get_cursor_x(void);
int get_cursor_y(void);
void kclear_screen();
int height(void);
int width(void);
void kscroll_screen(void);