#include <paging.h>
#include <kernel_api.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <colors.h>

#define PT_LOAD 1

#define PF_X 1
#define PF_W 2
#define PF_R 4

#define USER_BASE 0x400000ULL
#define USER_STACK 0x800000ULL

struct elf64_ehdr {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

extern void enter_userspace(uint64_t rip, uint64_t rsp);

static int elf_load_user(const uint8_t *elf, uint64_t *entry, uint64_t *user_stack, KernelAPI *api) {
    const struct elf64_ehdr *ehdr = (const struct elf64_ehdr*)elf;

    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        return -1;
    }

    if (ehdr->e_ident[4] != 2) {
        return -1;
    }

    const struct elf64_phdr *phdr = (const struct elf64_phdr*)(elf + ehdr->e_phoff);

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        uint64_t vaddr = phdr[i].p_vaddr;
        uint64_t filesz = phdr[i].p_filesz;
        uint64_t memsz = phdr[i].p_memsz;

        if (memsz < filesz) {
            return -1;
        }

        uint64_t start = vaddr & ~0xFFFULL;

        uint64_t end = (vaddr + memsz + 0xFFFULL) & ~0xFFFULL;

        uint64_t flags = PAGE_PRESENT | PAGE_USER;

        if (phdr[i].p_flags & PF_W) {
            flags |= PAGE_WRITE;
        }

        if (!(phdr[i].p_flags & PF_X)) {
            flags |= PAGE_NX;
        }

        for (uint64_t addr = start; addr < end; addr += 0x1000) {
            uint64_t phys = api->pmm_alloc_pages(1);

            if (!phys) {
                return -1;
            }

            uint64_t load_flags = PAGE_USER | PAGE_WRITE;

            api->user_map_page(addr, phys, load_flags);

            uint64_t pte = api->get_pte(addr);

            /*
            api->kprintf(
                "ELF flags=%x load_flags=%x PTE=%p\n",
                phdr[i].p_flags,
                load_flags,
                (void*)pte
            ); */

            // api->kprintf("VA= %p PA=%p PTE=%p\n", (void*)addr, (void*)phys, (void*)pte);

            // api->kprintf("segment %d: flags=%x vaddr=%p filesz=%p memsz=%p\n", i, phdr[i].p_flags, (void*)phdr[i].p_vaddr, (void*)phdr[i].p_filesz, (void*)phdr[i].p_memsz);

            volatile uint8_t *p = (volatile uint8_t *)0x400000;

            p[482] = 0;
            api->kprintf("482 OK\n");

            p[483] = 0;
            api->kprintf("483 OK\n");

            while (1) {}

            api->memset((void*)addr, 0, 0x1000); // Crash
        }

        api->memcpy((void*)vaddr, elf + phdr[i].p_offset, filesz);
    }

    uint64_t stack_phys = api->pmm_alloc_pages(1);

    if (!stack_phys) {
        return -1;
    }

    api->map_page(USER_STACK - 0x1000, stack_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    *entry = ehdr->e_entry;
    *user_stack = USER_STACK;

    return 0;
}

bool kernel_load_user(const char *path, KernelAPI *api) {
    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Loading userspace module %s...\n", path);

    LoadedFile file = api->fat32_load_file(path);
    
    if (!file.data) {
        api->kprintf(WHITE "<" RED " ERROR " WHITE "> " "Failed to load userspace module %s!\n", path);
        return false;
    }

    uint64_t entry;
    uint64_t user_stack;

    if (elf_load_user(file.data, &entry, &user_stack, api) != 0) { // Crash
        api->kprintf(WHITE "<" RED " ERROR " WHITE "> " "Userspace ELF load failed!\n");

        api->free(file.data);

        return false;
    }

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Userspace entry: %p\n", (void* )entry);

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Userspace stack: %p\n", (void* )user_stack);

    enter_userspace(entry, user_stack);

    api->kprintf(WHITE "<" RED " ERROR " WHITE "> " "Userspace %s returned!\n", path);

    api->free(file.data);

    return true;
}