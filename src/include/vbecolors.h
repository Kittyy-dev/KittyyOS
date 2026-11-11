#include <stdint.h>

#define VBE_BLACK        "\x01"
#define VBE_RED          "\x02"
#define VBE_GREEN        "\x03"
#define VBE_YELLOW       "\x04"
#define VBE_BLUE         "\x05"
#define VBE_MAGENTA      "\x06"
#define VBE_CYAN         "\x07"
#define VBE_WHITE        "\x08"

#define VBE_BRIGHT_BLACK   "\x11"
#define VBE_BRIGHT_RED     "\x12"
#define VBE_BRIGHT_GREEN   "\x13"
#define VBE_BRIGHT_YELLOW  "\x14"
#define VBE_BRIGHT_BLUE    "\x15"
#define VBE_BRIGHT_MAGENTA "\x16"
#define VBE_BRIGHT_CYAN    "\x17"
#define VBE_BRIGHT_WHITE   "\x18"

static const uint32_t vbe_dark_colors[] = {
    0x000000, // black
    0xAA0000, // red
    0x00AA00, // green
    0xAA5500, // yellow
    0x0000AA, // blue
    0xAA00AA, // magenta
    0x00AAAA, // cyan
    0xAAAAAA  // white (light gray)
};

static const uint32_t vbe_bright_colors[] = {
    0x555555, // bright black (dark gray)
    0xFF5555, // bright red
    0x55FF55, // bright green
    0xFFFF55, // bright yellow
    0x5555FF, // bright blue
    0xFF55FF, // bright magenta
    0x55FFFF, // bright cyan
    0xFFFFFF  // bright white
};
