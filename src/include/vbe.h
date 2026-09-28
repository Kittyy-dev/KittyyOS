#ifndef VBE_H
#define VBE_H

#include <stdarg.h>
#include <stdint.h>

typedef struct {
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint32_t physbase;

    uint8_t *backbuffer;
    uint32_t backbuffer_size;
} VBEInfo;

/* --- Globals --- */
extern VBEInfo vbe;

extern uint32_t VBE_DEFAULT_FG;
extern uint32_t VBE_DEFAULT_BG;

extern uint32_t vbe_fg;
extern uint32_t vbe_bg;

/* --- Init & Mapping --- */
void map_framebuffer();
void vbe_init();

/* --- Color setters --- */
void vbe_set_fg(uint32_t fg);
void vbe_set_bg(uint32_t bg);

/* --- Pixel & drawing --- */
void vbe_putpixel(int x, int y, uint32_t color);
void vbe_draw_circle(int cx, int cy, int r, uint32_t color);

/* --- Character rendering --- */
void vbe_put_bg_at(int x, int y, uint32_t bg);
void vbe_putchar_at(int x, int y, char c, uint32_t color);

/* --- Text output --- */
void vbe_putc(char c);
void vbe_print(const char* s);
void vbe_printf(const char* fmt, va_list args);

/* --- Screen control --- */
void vbe_clear_screen();
void vbe_debug_glyph(char c);
void vbe_test_rect(void);
void vbe_move_cursor(int x, int y);
void gradientTest(void);
void atom_putpixel(uint8_t *buffer, int x, int y, uint32_t color);
void vbe_scroll_screen(void);

void vbe_print_at(int x, int y, const char *s, uint32_t color);

#endif
