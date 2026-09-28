#include "elf.h"
#include <stdint.h>
#include <vfs.h>
#include <fat.h>
#include <vbe.h>
#include <heap.h>
#include <kernel_api.h>

#include <kprint.h>
#include <stdlib.h>
#include <heap.h>
#include <keyboard.h>

#include <tools.h>
#include <cd.h>
#include <string.h>
#include <ports.h>
#include <bios.h>
#include <cpu.h>
#include <kernel.h>
#include <context.h>
#include <tools.h>
#include <vga.h>
#include <paging.h>
#include <tsc.h>
#include <pmm.h>

extern VBEInfo vbe;

extern uint64_t get_memory_mb(void);

bool fat32_load_file_api(
    const char *path,
    uint8_t **data,
    uint32_t *size
) {
    kprintf("1 path=%s\n", path);

    LoadedFile file = fat32_load_file(path);

    kprintf("2\n");

    if (!file.data) {
        return false;
    }

    *data = file.data;
    *size = file.size;

    kprintf("3 data=%p size=%u\n", file.data, file.size);

    return true;
}

KernelAPI kernel_api = {

    // TTY
    .kprintf = kprintf,
    .kclear_screen = kclear_screen,
    .vbe_putpixel = vbe_putpixel,
    .vbe_putchar_at = vbe_putchar_at,
    .vbe_print_at = vbe_print_at,
    .vga_print_at = vga_print_at,
    .putchar_at = putchar_at,
    .vbe_draw_circle = vbe_draw_circle,
    .atom_putpixel = atom_putpixel,

    // Memory
    .kmalloc = kmalloc,
    .free = free,
    .heap_used = heap_used,
    .heap_size = heap_size,
    .virt_to_phys = virt_to_phys,
    .map_page = map_page,
    .pmm_alloc_pages = pmm_alloc_pages,
    .user_map_page = user_map_page,
    .get_pte = get_pte,

    // String
    .strcmp = strcmp,
    .strncmp = strncmp,
    .strlen = strlen,
    .strcpy = strcpy,
    .memset = memset,
    .memcpy = memcpy,
    .strdup = strdup,
    .strncpy = strncpy,

    // Filesystem
    .fat32_ls = fat32_ls,
    .fat32_cd = fat32_cd,
    .fat32_show = fat32_show,
    .fat32_get_current_dir_name = fat32_get_current_dir_name,
    .fat32_load_file = fat32_load_file,

    // Input
    .shell_readline = shell_readline,
    .get_char_from_keyboard = get_char_from_keyboard,
    .get_char_from_keyboard_nonblocking = get_char_from_keyboard_nonblocking,

    // Hardware
    .inb = inb,
    .outb = outb,

    // System
    .acpi_shutdown = acpi_shutdown,

    // CPU
    .cpu_brand = cpu_brand,
    .cpu_vendor = cpu_vendor,
    .cpuid = cpuid,
    .cpuid_max_leaf = cpuid_max_leaf,
    .cpu_stepping = cpu_stepping,
    .cpu_family = cpu_family,
    .cpu_model = cpu_model,
    .cpu_apic_id = cpu_apic_id,
    .cpuid_max_extended_leaf = cpuid_max_extended_leaf,
    // .cpu_id = cpu_id,


    // Cursor
    .get_cursor_x = get_cursor_x,
    .get_cursor_y = get_cursor_y,

    // Infos
    .hostname = g_hostname,
    .get_memory_mb = get_memory_mb,

    // Proc
    .register_scheduler = register_scheduler,
    .context_switch = kernel_context_switch,

    // Tools
    .kernel_load_module = kernel_load_module,

    // sleep
    .sleep_ms = sleep_ms,
    .tsc_timeout = tsc_timeout,
    .rdtsc = rdtsc,

    // struct
    .vbe = &vbe,
    

    // vfs
    .vfs_get_cwd = vfs_get_cwd,
    .vfs_cd = vfs_cd,
    .vfs_get_root = vfs_get_root,
    .vfs_resolve_path = vfs_resolve_path,
    .vfs_mount = vfs_mount,

    .vfs_create_file = vfs_create_file,
    .vfs_create_dir = vfs_create_dir,

    .vfs_read = vfs_read,
    .vfs_write = vfs_write,
    .vfs_delete = vfs_delete,

    .vfs_register = vfs_register,
    .vfs_mkdir = vfs_mkdir,
    .vfs_ls = vfs_ls,
    .elf_load = elf_load,
    .fat32_load_file_api = fat32_load_file_api,

    // tsc

    .get_tsc_freq = get_tsc_freq
};