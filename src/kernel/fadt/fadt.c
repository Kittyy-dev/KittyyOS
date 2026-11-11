#include "vfs.h"
#include <stdint.h>
#include <string.h>
#include <paging.h>
#include <bios.h>
#include <ports.h>
#include <vga.h>
#include <colors.h>
#include <tsc.h>
#include <kprint.h>

void acpi_map_bios_region(void) {
    for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    }
}

void* find_rsdp() {
    for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp((char*)(uintptr_t)addr, "RSD PTR ", 8) == 0) {
            return (void*)(uintptr_t)addr;
        }
    }
    return 0;
}

struct FADT* get_fadt() {
    struct RSDPDescriptor20* rsdp = find_rsdp();
    if (!rsdp) {
        log_error("RSDP not found!\n");
        return 0;
    }

    log_kernel("RSDP revision = %d\n", rsdp->revision);
    log_kernel("RSDT addr = 0x%x\n", rsdp->rsdt_address);
    log_kernel("XSDT addr = 0x%lx\n", rsdp->xsdt_address);

    if (rsdp->revision == 0) {
        uint32_t rsdt_addr = rsdp->rsdt_address;

        if (!rsdt_addr) {
            log_error("RSDT address is NULL!\n");
            return 0;
        }

        log_kernel("Using RSDT at 0x%x\n", rsdt_addr);

        map_page(rsdt_addr & ~0xFFF, rsdt_addr & ~0xFFF, PAGE_RW);

        struct RSDT* rsdt = (struct RSDT*)(uintptr_t)rsdt_addr;

        int entries = (rsdt->length - sizeof(struct RSDT)) / 4;

        for (int i = 0; i < entries; i++) {
            uint32_t table_addr = rsdt->entries[i];

            map_page(table_addr & ~0xFFF, table_addr & ~0xFFF, PAGE_RW);

            char* sig = (char*)(uintptr_t)table_addr;

            if (memcmp(sig, "FACP", 4) == 0) {
                log_kernel("FADT found at 0x%x\n", table_addr);
                return (struct FADT*)sig;
            }
        }

        log_error("FADT not found in RSDT!\n");
        return 0;
    }

    uint64_t xsdt_addr = rsdp->xsdt_address;
    if (!xsdt_addr) {
        log_error("XSDT missing in ACPI 2.0+\n");
        return 0;
    }

    log_kernel("Using XSDT 0x%lx\n", xsdt_addr);

    map_page(xsdt_addr & ~0xFFF, xsdt_addr & ~0xFFF, PAGE_RW);

    struct XSDT* xsdt = (struct XSDT*)(uintptr_t)xsdt_addr;

    int entries = (xsdt->length - sizeof(struct XSDT)) / 8;

    for (int i = 0; i < entries; i++) {
        uint64_t table_addr = xsdt->entries[i];

        map_page(table_addr & ~0xFFF, table_addr & ~0xFFF, PAGE_RW);

        char* sig = (char*)(uintptr_t)table_addr;

        if (memcmp(sig, "FACP", 4) == 0) {
            log_kernel("FADT found @ 0x%lx\n", table_addr);
            return (struct FADT*)sig;
        }
    }

    log_error("FADT not found in XSDT!\n");
    return 0;
}

void acpi_shutdown(void) {
    kclear_screen();
    vfs_umount_all();
    log_kernel("VFS umounted!\n");
    struct FADT* fadt = get_fadt();
    if (!fadt) return;

    uint16_t pm1b = fadt->pm1b_control_block;
    if (!pm1b) return;

    uint16_t SLP_EN  = 1 << 13;
    uint16_t SLP_TYP = 5 << 10;

    uint16_t val = SLP_TYP | SLP_EN;

    sleep_ms(500);

    outw(pm1b, val);
    outw(0x604, 0x2000);

    for (;;) {
        asm volatile("hlt");
    }
}