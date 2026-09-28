#include <paging.h>
#include <vga.h>
#include <stdint.h>
#include <kernel_api.h>
#include <colors.h>
#include <bitmap.h>
#include <vbe.h>
#include <font.h>

#define KEY_UP     ((char)0xF1)
#define KEY_DOWN   ((char)0xF2)
#define KEY_LEFT   ((char)0xF3)
#define KEY_RIGHT  ((char)0xF4)

#define OPTION_COUNT 4

int selected = 0;

const char *options[] = {
    "Boot KittyyOS",
    "Memory Test",
    "Sytem Info",
    "Shutdown"
};

#define BOOT_INFO_ADDR 0x3000

#define VIDEO_MODE_TEXT 0x0
#define VIDEO_MODE_VBE  0x1

typedef struct
{
    uint32_t video_mode;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint64_t framebuffer;

} BootInfo;

static volatile BootInfo* boot_info = (volatile BootInfo*)BOOT_INFO_ADDR;

KernelAPI *api;

void fbPrintBitmap(int x, int y, int height, int width, const uint32_t *bitmap) { 
    if (bitmap == NULL) {
        return;
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int px = x + j;
            int py = y + i;

            if (px < 0 || py < 0 || px >= (int)api->vbe->width || py >= (int)api->vbe->height) {
                continue;
            }

            api->vbe_putpixel(px, py, bitmap[i * width + j]);
        }
    }
}

void fbPrintBitmapScaled(int x, int y, int src_width, int src_height, int dst_width, int dst_height, const uint32_t *bitmap) {
    if (bitmap == NULL) {
        return;
    }

    for (int y2 = 0; y2 < dst_height; y2++) {
        for (int x2 = 0; x2 < dst_width; x2++) {

            int px = x + x2;
            int py = y + y2;

            if (px < 0 || py < 0 || px >= (int)api->vbe->width || py >= (int)api->vbe->height) {
                continue;
            }

            int src_x = ((uint64_t)x2 * src_width) / dst_width;
            int src_y = ((uint64_t)y2 * src_height) / dst_height;

            uint32_t pixel = bitmap[src_y * src_width + src_x];

            api->vbe_putpixel(px, py, pixel);
        }
    }
}

bool memory_test(uintptr_t start, uintptr_t end) {
    const uint32_t patterns[] = {
        0x00000000,
        0xFFFFFFFF,
        0xAAAAAAAA,
        0x55555555
    };

    volatile uint32_t *ptr = (volatile uint32_t*)start;

    volatile uint32_t *last = (volatile uint32_t*)end;

    for (int p = 0; p < 4; p++) {
        uint32_t pattern = patterns[p];

        for (; ptr < last; ptr++) {
            *ptr = pattern;
        }

        ptr = (volatile uint32_t*)start;

        for (; ptr < last; ptr++) {
            if (*ptr != pattern) {
                return false;
            }
        }

        ptr = (volatile uint32_t*)start;
    }

    return true;
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < width; i++) {
        api->vbe_putpixel(x + i, y, color);
    }

    for (int i = 0; i < width; i++) {
        api->vbe_putpixel(x + i, y + height - 1, color);
    }

    for (int i = 0; i <height; i++) {
        api->vbe_putpixel(x, y + i, color);
    }

    for (int i = 0; i < height; i++) {
        api->vbe_putpixel(x + width - 1, y + i, color);
    }  
}

void draw_rect_text(int x, int y, int width, int height, uint8_t color) {
    while (1) {}
    for (int i = 0; i < width; i++) {
        api->putchar_at(x + i, y, '+', color);
        api->putchar_at(x + i, y + height - 1, '+', color);
    }

    for (int i = 0; i < height; i++) {
        api->putchar_at(x, y + i, '|', color);
        api->putchar_at(x + width - 1, y + i, '|', color);
    }
}

void disable_cursor(void) {
    api->outb(0x3D4, 0x0A);
    api->outb(0x3D5, 0x20);
}

void enable_cursor(void) {
    api->outb(0x3D4, 0x0A);
    api->outb(0x3D5, 0x0E);

    api->outb(0x3D4, 0x0B);
    api->outb(0x3D5, 0x0F);
}

void draw_menu_text(void) {
    api->kclear_screen();

    const char *title = "Welcome to KittyyOS!";

    int title_x = (80 - api->strlen(title)) / 2;
    int title_y = 25 / 8;

    api->vga_print_at(title_x, title_y, title, 0x0F);

    int menu_width = 30;
    int menu_height = OPTION_COUNT + 4;

    int menu_x = (80 - menu_width) / 2;
    int menu_y = (title_y + 3);

    for (int i = 0; i < OPTION_COUNT; i++) {
        const char *text = options[i];

        int text_width = api->strlen(text);

        int x = (80 - text_width) / 2;
        int y = menu_y + 1 + i;

        if (i == selected) {
            api->vga_print_at(x - 2, y, "> ", 0x0F);
        }

        api->vga_print_at(x, y, text, 0x0F);
    }

    // draw_rect_text(menu_x, menu_y, menu_width, menu_height, 0x0F);
}

void draw_menu_vbe(void) {
    api->kclear_screen();

    const char *title = "Welcome to KittyyOS!";

    int title_x = (api->vbe->width - api->strlen(title) * CHAR_WIDTH) / 2;

    int title_y = api->vbe->height / 8;

    api->vbe_print_at(title_x, title_y, title, 0xFFFFFF);

    // menu

    int menu_width = 300;
    int menu_height = OPTION_COUNT * (CHAR_HEIGHT + 10) + CHAR_HEIGHT;

    int menu_y = title_y + CHAR_HEIGHT * 3;
    int menu_x = (api->vbe->width - menu_width) / 2;

    for (int i = 0; i < OPTION_COUNT; i++) {
        const char *text = options[i];

        int text_width = api->strlen(text) * CHAR_WIDTH;

        int x = (api->vbe->width - text_width) / 2;

        int y = menu_y + i * (CHAR_HEIGHT + 10);

        if (i == selected) {
            api->vbe_print_at(x - CHAR_WIDTH * 2, y, "> ", 0xFFFFFF);
        }

        api->vbe_print_at(x, y, text, 0xFFFFFF);
    }

    draw_rect(menu_x, menu_y - CHAR_HEIGHT, menu_width, menu_height, 0xFFFFFF);
}

void draw_menu() {
    if (boot_info->video_mode == VIDEO_MODE_VBE) {
        draw_menu_vbe();
    } else if (boot_info->video_mode == VIDEO_MODE_TEXT) {
        disable_cursor();
        draw_menu_text();
    }
}

void memory_test_screen(void) {
    api->kclear_screen();

    api->kprintf("Testing Memory...\n");

    bool result = memory_test(0x01800000, 0x01900000);

    if (result) {
        api->kprintf("Memory Test: OK!\n");
    } else {
        api->kprintf("Memory Test: FAILED!\n");
    }

    api->kprintf("Press any key to return\n");

    api->get_char_from_keyboard();

    draw_menu();
}

void menu_loop(void) {
    draw_menu();

    while (1) {
        char key = api->get_char_from_keyboard();

        if (key == KEY_UP) {
            if (selected > 0) {
                selected--;
            }

            draw_menu();
        } else if (key == KEY_DOWN) {
            if (selected < OPTION_COUNT - 1) {
                selected++;
            }

            draw_menu();
        } else if (key == '\n') {
            api->kclear_screen();
            
            switch (selected) {
                case 0:
                    continue;
                
                case 1:
                    memory_test_screen();
                    break;
                
                case 2:
                    int mb = api->get_memory_mb();
                    char cpuVendor[13];
                    char cpuBrand[20];

                    char *vesar_string = (char*)0x4400;

                    api->cpu_vendor(cpuVendor);
                    api->cpu_brand(cpuBrand);

                    api->kprintf("OS: KittyyOS\n");
                    api->kprintf("Kernel: KittyyOS Kernel x86_64 Version 1.0\n");
                    api->kprintf("Bootloader: KittyyOS Bootloader\n");
                    api->kprintf("Memory: %dMB\n", mb);
                    api->kprintf("CPU: %s %s\n", cpuVendor, cpuBrand);
                    api->kprintf("VESA OEM: %s\n", vesar_string);

                    break;
                
                case 3:
                    api->acpi_shutdown();
                    break;
            }
        }
    }
}

uint64_t get_rip(void) {
    uint64_t rip;

    asm volatile("leaq (%%rip), %0" : "=r"(rip));

    return rip;
}

int f2(int x) {
    return (x * -10);
}

void vbe_draw_fx(int (*func)(int), int scale, uint32_t color)
{
    int origin_x = 0;                         
    int origin_y = api->vbe->height - 1;     

    for (int x = 0; x <= api->vbe->width + api->vbe->height; x++)
    {
        int y = func(x); // y = x

        int screen_x = origin_x + x * scale;
        int screen_y = origin_y - y * scale;

        if (screen_x < 0 || screen_x >= (int)api->vbe->width ||
            screen_y < 0 || screen_y >= (int)api->vbe->height)
            continue;

        api->vbe_putpixel(screen_x, screen_y, color);
    }
}

void hh_entry(KernelAPI *kernel_api) {
    api = kernel_api;

    uint64_t rip = get_rip();
    uint64_t phys = api->virt_to_phys(rip);

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Higher Half Mapping enabled!\n");
    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "RIP Virtual Addres: 0x%llx\n", rip);
    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "RIP Phys Address: 0x%llx\n", phys);
    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Press m for menu!\n");

    uint64_t start = api->rdtsc();

    while (!api->tsc_timeout(1000, start)) {
        char c = api->get_char_from_keyboard_nonblocking();

        if (c == 'm') {
            menu_loop();
            return;
        }

        if (c != 0) {
            break;
        }

        __asm__ volatile("pause");
    }

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Starting Scheduler...\n");


    // api->kclear_screen();
    // vbe_draw_fx(f2, 1, 0x0000FF00);

    api->kernel_load_module("/bin/sched.bin");

    // draw_menu();

    // api->kprintf("%d x %d", api->vbe->width, api->vbe->height);

    // fbPrintBitmapScaled(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, api->vbe->width, api->vbe->height, image_pixels);

    // fbPrintBitmap(0, 0, IMAGE_HEIGHT, IMAGE_WIDTH, image_pixels);
    // fbPrintBitmap(0, 128, IMAGE_HEIGHT, IMAGE_WIDTH, image_pixels);
    // fbPrintBitmap(0, 256, IMAGE_HEIGHT, IMAGE_WIDTH, image_pixels);

    // api->kprintf("WIDTH: %f\n", vbe->width);
    // api->kprintf("HEIGHT: %f\n", vbe->height);

    // fbPrintBitmap(0, 0, IMAGE_HEIGHT, IMAGE_WIDTH, image_pixels);

    while (1) {}
}