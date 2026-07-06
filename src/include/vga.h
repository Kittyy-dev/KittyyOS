#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define BLACK_ON_BLACK 0x00
#define OUT_MAX 1024

extern uint8_t DEFAULT_COLOR;

void clear_screen(void);
void vga_putc(char c);
void vga_print(const char* str);
void putchar_at(int x, int y, char c, uint8_t color);
void move_cursor(int x, int y);
void scroll_screen(void);
void kernel_info(const char* info);
void itoa(int value, char* str, int base);
void vga_printf(const char* fmt, ...);

extern int cursor_x;
extern int cursor_y;

#endif