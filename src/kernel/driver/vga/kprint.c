#include <gop.h>
#include <font.h>
#include <stdint.h>
#include <vbe.h>
#include <vga.h>
#include <paging.h>
#include <kprint.h>
#include <kcolors.h>
#include <stdarg.h>

#define BOOT_INFO_ADDR 0x3000

#define VIDEO_MODE_TEXT 0x0
#define VIDEO_MODE_VBE  0x1
#define VIDEO_MODE_GOP 0x2

typedef struct
{
    uint32_t video_mode;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint64_t framebuffer;

} BootInfo;

extern int vbe_x;
extern int vbe_y;

int kcursor_x;
int kcursor_y;

static volatile BootInfo* boot_info = (volatile BootInfo*)BOOT_INFO_ADDR;

void check_video_mode(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        paging_init();
        vbe_init();

        vbe_clear_screen();
        vbe_print("Copyright (C) KittyyOS\n");
        vbe_print("Using VBE Mode!\n");
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        paging_init();
        clear_screen();
        vga_print("Copyright (C) KittyyOS\n");
        vga_print("Using VGA Textmode!\n");
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        paging_init();
        gop_init();
        gop_clear_screen();
        gop_print("Copyright (C) KittyyOS\n");
        gop_print("Using GOP Mode!\n");
        // gop_clear_screen();
        // gop_putchar_at(0, 0, 'T',  0xFFFFFF);
    }
}

void kputchar_at(int x, int y, char c, KColor color) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_putchar_at(x * CHAR_WIDTH, y * CHAR_HEIGHT, c, vbe_color_from_kcolor(color));
    } else  if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        putchar_at(x, y, c, vga_color_from_kcolor(color));
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        gop_putchar_at(x *CHAR_WIDTH, y * CHAR_HEIGHT, c, vbe_color_from_kcolor(color));
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
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        gop_clear_screen();
    }
}

void kputc(char c) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_putc(c);
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        vga_putc(c);
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        gop_putc(c);
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_printf(fmt, args);
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        vga_printf(fmt, args);
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        gop_printf(fmt, args);
    }

    va_end(args);
}

int height(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe.height;
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        return 25;
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        return boot_info->height;
    }

    return 1;
}

int width(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        return vbe.width;
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        return 80;
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        return boot_info->width;
    }

    return 1;
}

void kscroll_screen(void) {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        vbe_scroll_screen();
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        scroll_screen();
    } else if (boot_info->video_mode == VIDEO_MODE_GOP) {
        gop_scroll_screen();
    }
}