#include <bootinfo.h>
#include <colors.h>
#include <font.h>
#include <stdint.h>
#include <vbe.h>
#include <vga.h>
#include <paging.h>
#include <kprint.h>
#include <kcolors.h>
#include <stdarg.h>
#include <vbecolors.h>

#define BOOT_INFO_ADDR 0x3000

#define VIDEO_MODE_TEXT 0x0
#define VIDEO_MODE_VBE  0x1
#define VIDEO_MODE_GOP 0x2

extern int vbe_x;
extern int vbe_y;

int kcursor_x;
int kcursor_y;

void kputchar_at(int x, int y, char c, KColor color) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_putchar_at(x * CHAR_WIDTH, y * CHAR_HEIGHT, c, vbe_color_from_kcolor(color));
    } else  if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        putchar_at(x, y, c, vga_color_from_kcolor(color));
    }
}

int get_cursor_x(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe_x / CHAR_WIDTH;
    }

    return cursor_x;
}

int get_cursor_y(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe_y / CHAR_HEIGHT;
    }

    return cursor_y;
}

void kcursor(int x, int y) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_x = x * CHAR_WIDTH;
        vbe_y = y * CHAR_HEIGHT;
        vbe_move_cursor(vbe_x, vbe_y);
    } else {
        move_cursor(x, y);
        cursor_x = x;
        cursor_y = y;
    }
}

void kclear_screen() {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_clear_screen();
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        clear_screen();
    }
}

void kputc(char c) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_putc(c);
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        vga_putc(c);
    }
}

void kprint(const char *s) {
    while (*s) {

        uint8_t code = (uint8_t)*s;

        if (boot_info->video_mode == VIDEO_MODE_VBE) {
            if (code >= 0x01 && code <= 0x08) {
                VBE_DEFAULT_FG = vbe_dark_colors[code - 1];
                s++;
                continue;
            }

            if (code >= 0x11 && code <= 0x18) {
                VBE_DEFAULT_FG = vbe_bright_colors[code - 0x11];
                s++;
                continue;
            }

            kputc(*s++);
        } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
            if (code >= 0x01 && code <= 0x08) {
                DEFAULT_COLOR = vga_colors[code - 1];
                s++;
                continue;
            }
            
            if (code >= 0x11 && code <= 0x18) {
                DEFAULT_COLOR = vga_colors[code - 0x11 + 8];
                s++;
                continue;
            }

            vga_putc(*s++);
        }
    }
}

void check_video_mode(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        boot_info_relocate();
        paging_init();
        vbe_init();

        vbe_clear_screen();
        kprint("Copyright (C) KittyyOS\n");
        kprint("Using VBE Mode!\n");
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        boot_info_relocate();
        paging_init();
        clear_screen();
        kprint("Copyright (C) KittyyOS\n");
        kprint("Using VGA Textmode!\n");
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *str = fmt_func(fmt, args);
    kprint(str);

    va_end(args);
}

int height(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe.height;
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        return 25;
    }

    return 1;
}

int width(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe.width;
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        return 80;
    }

    return 1;
}

void kscroll_screen(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_scroll_screen();
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        scroll_screen();
    }
}

void log_kernel(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *str = fmt_func(fmt, args);

    va_end(args);

    static char out[BUF_SIZE];

    size_t i = 0;
    append_string(out, &i, WHITE "<" GREEN " KERNEL " WHITE "> ");
    append_string(out, &i, str);
    out[i] = '\0';

    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        kprint(out);
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        kprint(out);
    }
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *str = fmt_func(fmt, args);

    va_end(args);

    static char out[BUF_SIZE];

    size_t i = 0;
    append_string(out, &i, WHITE "<" RED " ERROR " WHITE "> ");
    append_string(out, &i, str);
    out[i] = '\0';

    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        kprint(out);
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        kprint(out);
    }
}