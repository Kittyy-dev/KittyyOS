#ifndef GOP_H
#define GOP_H

#include <stdarg.h>
#include <stdint.h>

void gop_init(void);
void gop_clear_screen(void);
void gop_putchar_at(int x, int y, char c, uint32_t color);
void gop_putc(char c);
void gop_print(const char *str);
void gop_move_cursor(int x, int y);
void gop_putpixel(int x, int y, uint32_t color);
void gop_printf(const char *fmt, va_list args);
void gop_scroll_screen(void);

extern uint32_t VBE_DEFAULT_FG;
extern uint32_t VBE_DEFAULT_BG;

#endif