#include <string.h>
#include <stdint.h>

#define KPRINT_BUFFER_SIZE 1024

#define KCOLOR_RESET "\033[0m"
#define KCOLOR_BLACK "\033[0;30m"
#define KCOLOR_RED "\033[0;31m"
#define KCOLOR_GREEN "\033[0;32m"
#define KCOLOR_YELLOW "\033[0;33m"
#define KCOLOR_BLUE "\033[0;34m"
#define KCOLOR_MAGENTA "\033[0;35m"
#define KCOLOR_CYAN "\033[0;36m"
#define KCOLOR_WHITE "\033[0;37m"

#define KCOLOR_BRIGHT_BLACK "\033[1;30m"
#define KCOLOR_BRIGHT_RED "\033[1;31m"
#define KCOLOR_BRIGHT_GREEN "\033[1;32m"
#define KCOLOR_BRIGHT_YELLOW "\033[1;33m"
#define KCOLOR_BRIGHT_BLUE "\033[1;34m"
#define KCOLOR_BRIGHT_MAGENTA "\033[1;35m"
#define KCOLOR_BRIGHT_CYAN "\033[1;36m"
#define KCOLOR_BRIGHT_WHITE "\033[1;37m"

int krpint_is_color_code(const char* str);
uint8_t kprint_ansi_to_vbe(const char* str);
void kprint_vbe(const char* str);