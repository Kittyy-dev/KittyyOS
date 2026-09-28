#include <stdint.h>
#include <vbe.h>
#include <font.h>
#include <paging.h>
#include <string.h>
#include <vbecolors.h>
#include <vga.h>
#include <stdarg.h>
#include <kprint.h>

VBEInfo vbe;

int vbe_x = 0;
int vbe_y = 0;

uint32_t vbe_fg;
uint32_t vbe_bg;

uint32_t VBE_DEFAULT_FG = 0xFFFFFF;
uint32_t VBE_DEFAULT_BG = 0x000000;

void map_framebuffer() {
    uint32_t fb_addr = *(uint32_t*)(0x4000 + 0x28);
    uint16_t pitch   = *(uint16_t*)(0x4000 + 0x10);
    uint16_t height  = *(uint16_t*)(0x4000 + 0x14);

    uint64_t fb_size = (uint64_t)pitch * height;

    for (uint64_t addr = fb_addr; addr < fb_addr + fb_size; addr += 0x1000) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_RW);
    }
    // *(volatile uint32_t*)0xFD000000 = 0xFF0000;
}

void vbe_init() {
    uint8_t* info = (uint8_t*)0x4000;

    uint16_t pitch   = *(uint16_t*)(info + 0x10);
    uint16_t width   = *(uint16_t*)(info + 0x12);
    uint16_t height  = *(uint16_t*)(info + 0x14);
    uint8_t  bpp     = *(uint8_t*)(info + 0x19);
    uint32_t phys    = *(uint32_t*)(info + 0x28);

    vbe.pitch    = pitch;
    vbe.width    = width;
    vbe.height   = height;
    vbe.bpp      = bpp;
    vbe.physbase = phys;

    map_framebuffer();
}

uint32_t vbe_color_from_kcolor(KColor color) {
    if (color >= 16) {
        return vbe_colors[0];
    }

    return vbe_colors[color];
}

void vbe_test_rect(void)
{
    volatile uint8_t *fb =
        (volatile uint8_t *)(uintptr_t)vbe.physbase;

    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 100; x++) {

            uint32_t offset =
                y * vbe.pitch +
                x * (vbe.bpp / 8);

            if (vbe.bpp == 32) {
                *(volatile uint32_t *)(fb + offset) = 0x00FF0000;
            }
            else if (vbe.bpp == 24) {
                fb[offset + 0] = 0x00; // B
                fb[offset + 1] = 0x00; // G
                fb[offset + 2] = 0xFF; // R
            }
            else if (vbe.bpp == 16) {
                uint16_t color =
                    ((0xFF >> 3) << 11) |
                    ((0x00 >> 2) << 5)  |
                    ((0x00 >> 3) << 0);

                *(volatile uint16_t *)(fb + offset) = color;
            }
        }
    }
}

void vbe_set_fg(uint32_t fg) {
    vbe_fg = fg;
    VBE_DEFAULT_FG = fg;
} 

void vbe_set_bg(uint32_t bg) {
    vbe_bg = bg;
    VBE_DEFAULT_BG = bg;
}

void vbe_putpixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 ||
        x >= vbe.width ||
        y >= vbe.height)
    {
        return;
    }

    volatile uint8_t *fb =
        (volatile uint8_t *)(uintptr_t)vbe.physbase;

    uint32_t offset =
        y * vbe.pitch +
        x * (vbe.bpp / 8);

    if (vbe.bpp == 32)
    {
        *(volatile uint32_t *)(fb + offset) = color;
    }
    else if (vbe.bpp == 24)
    {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8)  & 0xFF;
        uint8_t b = color & 0xFF;

        // BGR
        fb[offset + 0] = b;
        fb[offset + 1] = g;
        fb[offset + 2] = r;
    }
    else if (vbe.bpp == 16)
    {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8)  & 0xFF;
        uint8_t b = color & 0xFF;

        uint16_t rgb565 =
            ((r >> 3) << 11) |
            ((g >> 2) << 5) |
            (b >> 3);

        *(volatile uint16_t *)(fb + offset) = rgb565;
    }
}

void atom_putpixel(uint8_t *buffer, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= vbe.width || y >= vbe.height) {
        return;
    }

    uint32_t offset = y * vbe.pitch + x * (vbe.bpp / 8);

    if (vbe.bpp == 32) {
        *(uint32_t *)(buffer + offset) = color;
    } else if (vbe.bpp == 24) {
        buffer[offset + 0] = color & 0xFF;
        buffer[offset + 1] = (color >> 8) & 0xFF;
        buffer[offset + 2] = (color >> 16) & 0xFF;
    } else if (vbe.bpp == 16) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;

        uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        *(uint16_t *)(buffer + offset) = rgb565;
    }
}

void vbe_draw_circle(int cx, int cy, int r, uint32_t color) {
    int r2 = r * r;

    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r2) {
                vbe_putpixel(cx + x, cy + y, color);
            }
        }
    }
}

void vbe_put_bg_at(int x, int y, uint32_t bg) {
    for (int row = 0; row < CHAR_HEIGHT; row++) {
        for (int col = 0; col < CHAR_WIDTH; col++) {
            vbe_putpixel(x + col, y + row, bg);
        }
    }
}

void vbe_putchar_at(int x, int y, char c, uint32_t color) {
    const uint8_t* glyph = &vgafont[(uint8_t)c * CHAR_HEIGHT];
    vbe_put_bg_at(x, y, VBE_DEFAULT_BG);

    for (int row = 0; row < CHAR_HEIGHT; row++) {
        uint8_t bits = glyph[row];

        for (int col = 0; col < CHAR_WIDTH; col++) {
            if (bits & (1 << (7 - col))) {
                vbe_putpixel(x + col, y + row, color);
            }
        }
    }
}

void vbe_print_at(int x, int y, const char *s, uint32_t color) {
    if (s == NULL) {
        return;
    }

    int cursor_x = x;
    int cursor_y = y;

    while (*s) {
        if (*s == '\n') {
            cursor_x = x;
            cursor_y += CHAR_HEIGHT;
            s++;
            continue;
        }

        vbe_putchar_at(cursor_x, cursor_y, *s, color);

        cursor_x += CHAR_WIDTH;

        if (cursor_x + CHAR_WIDTH > vbe.width) {
            cursor_x = x;
            cursor_y += CHAR_HEIGHT;
        }

        if (cursor_y + CHAR_HEIGHT > vbe.width) {
            cursor_x = x;
            cursor_y += CHAR_HEIGHT;
        }

        if (cursor_y + CHAR_HEIGHT > vbe.height) {
            break;
        }

        s++;
    }
}

void vbe_move_cursor(int x, int y) {
    vbe_x = x;
    vbe_y = y;
}

void vbe_clear_screen() {
    for (int y = 0; y < vbe.height; y++) {
        for (int x = 0; x < vbe.width; x++) {
            vbe_putpixel(x, y, VBE_DEFAULT_BG);
        }
    }
    
    vbe_x = 0;
    vbe_y = 0;
}

void vbe_scroll_screen(void) {
    volatile uint8_t *fb = (volatile uint8_t*)(uintptr_t)vbe.physbase;
    
    uint32_t bytes_per_pixel = vbe.bpp / 8;

    uint32_t scroll_pixels = CHAR_HEIGHT;

    uint32_t copy_size = (vbe.height - scroll_pixels) * vbe.pitch;

    for (uint32_t y = 0; y < vbe.height - scroll_pixels; y++) {
        volatile uint8_t* dst = fb + y * vbe.pitch;

        volatile uint8_t* src = fb + (y + scroll_pixels) * vbe.pitch;

        for (uint32_t x = 0; x < vbe.pitch; x++) {
            dst[x] = src[x];
        }
    }

    for (uint32_t y = vbe.height - scroll_pixels; y < vbe.height; y++) {
        for (uint32_t x = 0; x < vbe.width; x++) {
            vbe_putpixel(x, y, VBE_DEFAULT_BG);
        }
    } 
}

void vbe_putc(char c) {
    if (c == '\n') {
        vbe_x = 0;
        vbe_y += CHAR_HEIGHT;

        if (vbe_y + CHAR_HEIGHT > vbe.height) {
            vbe_scroll_screen();
            vbe_y -= CHAR_HEIGHT;
        }

        return;
    }

    vbe_putchar_at(vbe_x, vbe_y, c, VBE_DEFAULT_FG);

    vbe_x += CHAR_WIDTH;

    if (vbe_x + CHAR_WIDTH >= vbe.width) {
        vbe_x = 0;
        vbe_y += CHAR_HEIGHT;
    }

    if (vbe_y + CHAR_HEIGHT > vbe.height) {
        vbe_scroll_screen();
        vbe_y -= CHAR_HEIGHT;
    }
}

void vbe_print(const char* s) {
    while (*s) {

        uint8_t code = (uint8_t)*s;

        // Dark colors 0x01–0x08
        if (code >= 0x01 && code <= 0x08) {
            VBE_DEFAULT_FG = vbe_dark_colors[code - 1];
            s++;
            continue;
        }

        // Bright colors 0x11–0x18
        if (code >= 0x11 && code <= 0x18) {
            VBE_DEFAULT_FG = vbe_bright_colors[code - 0x11];
            s++;
            continue;
        }

        vbe_putc(*s++);
    }
}

void vbe_printf(const char* fmt, va_list args) {
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

    vbe_print(out);
}