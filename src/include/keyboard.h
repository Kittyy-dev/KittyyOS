#include <stdint.h>

#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_handler(void);
void flush_keyboard_buffer();
char get_char_from_keyboard();
void keyboard_handle_scancode(uint8_t sc);
void shell_readline(char *out, int max);

#endif