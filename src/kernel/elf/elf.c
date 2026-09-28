#include <elf.h>
#include <stdint.h>
#include <string.h>
#include <heap.h>
#include <kprint.h>
#include <colors.h>
#include <pmm.h>
#include <paging.h>

void *elf_load(void *file)
{
    struct elf64_header *header = file;

    // ELF Magic
    if (header->ident[0] != 0x7F ||
        header->ident[1] != 'E'  ||
        header->ident[2] != 'L'  ||
        header->ident[3] != 'F') {

        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Module is not an ELF!\n");

        return NULL;
    }

    struct elf64_program_header *ph = (void *)((uint64_t)file + header->phoff);

    for (int i = 0; i < header->phnum; i++) {
        if (ph[i].type != PT_LOAD) {
            continue;
        }

        uint64_t virt_start = ph[i].vaddr;
        uint64_t mem_size   = ph[i].memsz;
        uint64_t file_size  = ph[i].filesz;

        uint64_t page_start = virt_start & ~0xFFFULL;

        uint64_t page_end = (virt_start + mem_size + 0xFFFULL) & ~0xFFFULL;

        uint64_t pages = (page_end - page_start) / PAGE_SIZE;

        if (pages == 0) {
            continue;
        }

        uint64_t phys = pmm_alloc_pages(pages);

        if (!phys) {

            kprintf(WHITE "<" RED " ERROR " WHITE "> " "Could not allocate ELF memory!\n");

            return NULL;
        }

        for (uint64_t off = 0; off < pages * PAGE_SIZE; off += PAGE_SIZE) {
            uint64_t virt = page_start + off;

            uint64_t physical = phys + off;

            map_page(virt, physical, PAGE_RW);
        }

        memset((void *)page_start, 0, pages * PAGE_SIZE);

        memcpy((void *)(virt_start), (uint8_t *)file + ph[i].offset, file_size);
    }

    return (void *)header->entry;
}