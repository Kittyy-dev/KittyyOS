#include <vbecolors.h>
#include <stdint.h>
#include <gop.h>
#include <bootinfo.h>
#include <font.h>
#include <paging.h>
#include <string.h>
#include <stdarg.h>

#define BOOT_INFO_ADDR 0x3000

static volatile BootInfo *boot_info = (volatile BootInfo*)BOOT_INFO_ADDR;

static uint32_t *framebuffer;

int gop_x = 0;
int gop_y = 0;

void gop_init() {
    framebuffer = (uint32_t*)(uintptr_t)boot_info->framebuffer;

    uint64_t fb_start = boot_info->framebuffer & ~0xFFFULL;
    uint64_t fb_end = boot_info->framebuffer + (uint64_t)boot_info->pitch * boot_info->height;

    fb_end = (fb_end + 0xFFF) & ~0xFFFULL;

    for (uint64_t addr = fb_start; addr < fb_end; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    }
}

void gop_putpixel(int x, int y, uint32_t color) {
    if (x < 0 || y < 0) {
        return;
    }

    if ((uint32_t)x >= boot_info->width) {
        return;
    }

    if ((uint32_t)y >= boot_info->height) {
        return;
    }

    uint8_t *fb = (uint8_t*)framebuffer;

    uint32_t *pixel = (uint32_t*)(fb + (uint64_t)y * boot_info->pitch + (uint64_t)x * 4);

    *pixel = color;
}

void gop_putchar_at(int x, int y, char c, uint32_t color) {
    const uint8_t *glyph = &vgafont[(uint8_t)c * CHAR_HEIGHT];

    for (int row = 0; row < CHAR_HEIGHT; row++) {
        uint8_t bits = glyph[row];

        for (int col = 0; col < CHAR_WIDTH; col++) {
            if (bits & (0x80 >> col)) {
                gop_putpixel(x + col, y + row, color);
            }
        }
    }
}

void gop_clear_screen(void) {
    uint8_t *fb = (uint8_t*)(uintptr_t)boot_info->framebuffer;

    for (uint32_t y = 0; y < boot_info->height; y++) {
        uint32_t *row = (uint32_t*)(fb + (uint64_t)y * boot_info->pitch);

        for (uint32_t x = 0; x < boot_info->width; x++) {
            row[x] = 0x00000000;
        }
    }

    gop_x = 0;
    gop_y = 0;
}

void gop_scroll_screen(void) {
    volatile uint8_t *fb = (volatile uint8_t*)(uintptr_t)boot_info->framebuffer;

    uint32_t bytes_per_pixel = boot_info->bpp / 8;
    uint32_t scroll_pixels = CHAR_HEIGHT;
    uint32_t copy_size = (boot_info->height - scroll_pixels) * boot_info->pitch;

    for (uint32_t y = 0; y < boot_info->height - scroll_pixels; y++) {
        volatile uint8_t *dst = fb + y * boot_info->pitch;
        volatile uint8_t *src = fb + (y + scroll_pixels) * boot_info->pitch;

        for (uint32_t x = 0; x < boot_info->pitch; x++) {
            dst[x] = src[x];
        }
    }

    for (uint32_t y = boot_info->height - scroll_pixels; y < boot_info->height; y++) {
        for (uint32_t x = 0; x < boot_info->width; x++) {
            gop_putpixel(x, y, VBE_DEFAULT_FG);
        }
    }
}

void gop_putc(char c) {
    if (c == '\n') {
        gop_x = 0;
        gop_y += CHAR_HEIGHT;

        if (gop_y + CHAR_HEIGHT > boot_info->height) {
            gop_scroll_screen();
            gop_y -= CHAR_HEIGHT;
        }

        return;
    }

    gop_putchar_at(gop_x, gop_y, c, VBE_DEFAULT_FG);

    gop_x += CHAR_WIDTH;

    if (gop_x + CHAR_WIDTH >= boot_info->width) {
        gop_x = 0;
        gop_y += CHAR_HEIGHT;
    }

    if (gop_y + CHAR_HEIGHT > boot_info->height) {
        gop_scroll_screen();
        gop_y -= CHAR_HEIGHT;
    }
}

void gop_print(const char *str) {
    while (*str) {
        uint8_t code = (uint8_t)*str;

        // Dark colors 0x01-0x08
        if (code >= 0x01 && code <= 0x08) {
            VBE_DEFAULT_FG = vbe_dark_colors[code - 1];
            str++;
            continue;
        }

        // Bright colors 0x11-0x18
        if (code >= 0x11 && code <= 0x18) {
            VBE_DEFAULT_FG = vbe_bright_colors[code - 0x11];
            str++;
            continue;
        }

        gop_putc(*str++);
    }
}

void gop_printf(const char* fmt, va_list args) {
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

    gop_print(out);
}