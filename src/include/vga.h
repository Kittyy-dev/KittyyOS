#ifndef VGA_H
#define VGA_H

#include <stdarg.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define BLACK_ON_BLACK 0x00
#define OUT_MAX 1024

extern uint8_t DEFAULT_COLOR;

void clear_screen(void);
void vga_putc(char c);
void vga_print(const char* str);
void vga_printf(const char* fmt, va_list args);
void putchar_at(int x, int y, char c, uint8_t color);
void move_cursor(int x, int y);
void scroll_screen(void);
void kernel_info(const char* info);
void u64toa(uint64_t value, char *str, int base);
void vga_set_fg(uint8_t fg);
void vga_set_bg(uint8_t bg);
void reset();
void vga_put_color_space(uint8_t bg);
void vga_print_at(int x, int y, const char *text, uint8_t color);

extern int cursor_x;
extern int cursor_y;

#endif