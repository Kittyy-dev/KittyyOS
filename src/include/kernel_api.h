#pragma once 

#include <stddef.h>
#include <vbe.h>
#include <fat.h>
#include <scheduler.h>
#include <stdint.h>
#include <vga.h>
#include <ports.h>
#include <paging.h>
#include <pmm.h>
#include <vfs.h>

struct scheduler_ops;

struct task;

void kernel_schedule(void);

typedef struct vfs_node vfs_node_t;

typedef struct {

    // TTY
    void (*kprintf)(const char*, ...);
    void (*kclear_screen)(void);
    void (*vbe_putpixel)(int x, int y, uint32_t color);
    void (*vbe_putchar_at)(int x, int y, char c, uint32_t color);
    void (*vbe_print_at)(int x, int y, const char *s, uint32_t color);
    void (*vga_print_at)(int x, int y, const char *text, uint8_t color);
    void (*putchar_at)(int x, int y, char c, uint8_t color);
    void (*vbe_draw_circle)(int cx, int cy, int r, uint32_t color);
    void (*atom_putpixel)(uint8_t *buffer, int x, int y, uint32_t color);

    // Memory
    void* (*kmalloc)(uint32_t);
    void (*free)(void*);
    uint32_t (*heap_used)(void);
    uint32_t (*heap_size)(void);
    uint64_t (*virt_to_phys)(uint64_t virt);
    void (*map_page)(uint64_t virt, uint64_t phys, uint64_t flags);
    uint64_t (*pmm_alloc_pages)(uint64_t count);
    void (*user_map_page)(uint64_t virt, uint64_t phys, uint64_t flags);
    uint64_t (*get_pte)(uint64_t virt);

    // String
    int (*strcmp)(const char*, const char*);
    int (*strncmp)(const char*, const char*, int);
    int (*strlen)(const char*);
    char* (*strcpy)(char*, const char*);
    void* (*memset)(void* dest, int c, size_t n);
    void* (*memcpy)(void* dest, const void* src, size_t n);
    char* (*strdup)(const char* s);
    char* (*strncpy)(char *dest, const char *src, size_t n);

    // Filesystem
    void (*fat32_ls)(void);
    void (*fat32_cd)(const char*);
    void (*fat32_show)(const char*);
    void (*fat32_get_current_dir_name)(char*);

    LoadedFile (*fat32_load_file)(const char* path);

    // Input
    void (*shell_readline)(char*, int);
    char (*get_char_from_keyboard)(void);
    char (*get_char_from_keyboard_nonblocking)(void);

    // Hardware
    uint8_t (*inb)(uint16_t);
    void (*outb)(uint16_t,uint8_t);

    // System
    void (*acpi_shutdown)(void);

    // CPU
    void (*cpu_brand)(char*);
    void (*cpu_vendor)(char*);
    void (*cpuid)(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t* ecx, uint32_t* edx);
    uint32_t (*cpuid_max_leaf)(void);
    uint32_t (*cpu_stepping)(void);
    uint32_t (*cpu_family)(void);
    uint32_t (*cpu_model)(void);
    uint32_t (*cpu_apic_id)(void);
    uint32_t (*cpuid_max_extended_leaf)(void);
    // uint32_t (*cpu_id)(void);

    // Debug
    void (*check_idt)(void);
    void (*check_gdt)(void);

    // Cursor
    int (*get_cursor_x)(void);
    int (*get_cursor_y)(void);

    // Modul
    void (*register_scheduler)(struct scheduler_ops *ops);

    char* hostname;
    uint64_t (*get_memory_mb)(void);

    // Proc
    void (*context_switch)(struct task *old, struct task *new);

    // tools
    bool (*kernel_load_module)(const char *path);

    // sleep
    void (*sleep_ms)(uint32_t ms);
    bool (*tsc_timeout)(uint32_t timeout_ms, uint64_t start_tsc);
    uint64_t (*rdtsc)();

    // struct
    VBEInfo *vbe;
    LoadedFile file;

    // vfs
    vfs_node_t* (*vfs_get_cwd)(void);
    bool (*vfs_cd)(const char* path);
    vfs_node_t* (*vfs_get_root)(void);
    vfs_node_t* (*vfs_resolve_path)(const char* path);
    vfs_node_t* (*vfs_mount)(const char* path, const char* fsname);

    vfs_node_t* (*vfs_create_file)(vfs_node_t* dir, const char* name);

    vfs_node_t* (*vfs_create_dir)(vfs_node_t* dir, const char* name);

    size_t (*vfs_read)(vfs_node_t* node, void* buf, size_t len);

    size_t (*vfs_write)(vfs_node_t* node, const void* buf, size_t len);


    void (*vfs_delete)(vfs_node_t* node);

    void (*vfs_register)(filesystem_t* fs);
    vfs_node_t* (*vfs_mkdir)(vfs_node_t* parent, const char* name);
    void (*vfs_ls)(vfs_node_t* dir);
    void *(*elf_load)(void *file);
    bool (*fat32_load_file_api)(const char *path, uint8_t **data, uint32_t *size);

    // Tsc
    uint64_t (*get_tsc_freq)();

    // sched
    void (*yield)(void);
    
} KernelAPI;