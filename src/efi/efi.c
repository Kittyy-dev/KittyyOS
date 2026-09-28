#include <bootinfo.h>
#include <elfefi.h>
#include <efi.h>
#include <colorsefi.h>
#include <stdint.h>
#include <gopefi.h>
#include <pciefi.h>
#include <uefi_fs.h>
#include <stringefi.h>

#define EfiReservedMemoryType      0
#define EfiLoaderCode              1
#define EfiBootServicesCode        3
#define EfiBootServicesData        4
#define EfiRuntimeServicesCode     5
#define EfiRuntimeServicesData     6
#define EfiConventionalMemory      7
#define EfiUnusableMemory          8
#define EfiACPIReclaimMemory       9
#define EfiACPIMemoryNVS           10
#define EfiMemoryMappedIO          11
#define EfiMemoryMappedIOPortSpace 12
#define EfiPalCode                 13
#define EfiPersistentMemory        14

#define E820_MAP_ADDR   0x5000
#define E820_COUNT_ADDR 0x4FFE
#define BOOT_INFO_ADDR 0x3000
#define VIDEO_MODE_GOP  0x2

typedef void (*KERNEL_ENTRY)(void);

static EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

UINT64 efi_prepare_stack(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    const UINTN pages = 16;

    UINT64 max_address = 0x5FFFF;

    EFI_STATUS status = SystemTable->BootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, pages, &max_address);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to allocate bootloader stack!\r\n");

        return 0;
    }

    return max_address + pages * 0x1000;
}

static void efi_zero_memory(void *ptr, UINT64 size) {
    UINT64 qwords = size / 8;
    UINT64 rest = size % 8;

    __asm__ volatile("rep stosq" : "+D"(ptr), "+c"(qwords) : "a"(0) : "memory");

    UINT8 *p = (UINT8*)ptr;

    for (UINT64 i = 0; i < rest; i++) {
        p[i] = 0;
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    efi_clear_screen(SystemTable);

    // efi_set_color(system_table, EFI_WHITE, EFI_BLACK);

    efi_print(SystemTable, EFI_COLOR_WHITE "Copyright (C) KittyyOS\r\n");
    
    EFI_FILE_PROTOCOL *Root = 0;

    EFI_STATUS status = efi_open_volume(ImageHandle, SystemTable, &Root);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to open volume!\r\n");

        return status;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "FAT12 volume opened!\r\n");

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "Reading Kernel...\r\n");

    EFI_FILE_PROTOCOL *KernelFile = 0;

    status = efi_open_file(Root, L"kernel.elf", &KernelFile);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to load kernel!\r\n");

        return status;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "Kernel opened!\r\n");

    // ELF

    ELF64_HEADER elf;

    status = KernelFile->SetPosition(KernelFile, 0);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to seek kernel!\r\n");

        return status;
    }

    UINTN read_size = sizeof(ELF64_HEADER);

    status = KernelFile->Read(KernelFile, &read_size, &elf);

    if (status != EFI_SUCCESS || read_size != sizeof(ELF64_HEADER)) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to read ELF header!\r\n");

        return status;
    }

    // Check ELF Magic
    if(elf.e_ident[0] != 0x7F || elf.e_ident[1] != 'E' || elf.e_ident[2] != 'L' || elf.e_ident[3] != 'F') {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Kernel is not an ELF file!\r\n");

        return 1;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "Valid ELF64 kernel!\r\n");

    // ELF Information
    if (elf.e_ident[4] != 2) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Kernel is not ELF64!\r\n");

        return 1;
    }

    if (elf.e_machine != 0x3E) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Kernel is not x86_64!\r\n");

        return -1;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "ELF header parsed!\r\n");

    // PH
    ELF64_PROGRAM_HEADER ph;

    status = KernelFile->SetPosition(KernelFile, elf.e_phoff);

    if (status != EFI_SUCCESS) {
        return status;
    }

    read_size = sizeof(ELF64_PROGRAM_HEADER);

    status = KernelFile->Read(KernelFile, &read_size, &ph);

    if (status != EFI_SUCCESS || read_size != sizeof(ELF64_PROGRAM_HEADER)) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to read program header!\r\n");

        return status;
    }

    if (ph.p_type == PT_LOAD) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> Found PT_LOAD segment!\r\n");
    }

    // PT_LOAD

    status = KernelFile->SetPosition(KernelFile, ph.p_offset);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to seek to kernel segment!\r\n");

        return status;
    }
 
    UINTN segment_size = (UINTN)ph.p_filesz;

    status = KernelFile->Read(KernelFile, &segment_size, (void*)(uintptr_t)ph.p_paddr);

    if (status != EFI_SUCCESS || segment_size != (UINTN)ph.p_filesz) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to load kernel segment!\r\n");

        return status;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "Kernel segment loaded!\r\n");

    // Emptry BSS
    
    if (ph.p_memsz < ph.p_filesz) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Invalid ELF segment sizes!\r\n");
        return 1;
    }

    UINT64 bss_start = ph.p_paddr + ph.p_filesz;
    UINT64 bss_size  = ph.p_memsz - ph.p_filesz;

    // efi_zero_memory((void*)(uintptr_t)bss_start, bss_size);

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;

    status = SystemTable->BootServices->LocateProtocol(&gop_guid, 0, (void**)&gop);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to locate GOP!\r\n");
        return status;
    }

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "GOP found!\r\n");

    BootInfo *boot_info = (BootInfo*)(UINTN)BOOT_INFO_ADDR;

    boot_info->video_mode = VIDEO_MODE_GOP;
    boot_info->width = gop->Mode->Info->HorizontalResolution;
    boot_info->height = gop->Mode->Info->VerticalResolution;
    boot_info->pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    boot_info->framebuffer = gop->Mode->FrameBufferBase;
    boot_info->bpp = 32;

    UINTN memory_map_size = 0;
    UINTN map_key = 0;
    UINTN descriptor_size = 0;
    UINT32 descriptor_version = 0;

    status = SystemTable->BootServices->GetMemoryMap(&memory_map_size, 0, &map_key, &descriptor_size, &descriptor_version);

    if (status != EFI_BUFFER_TOO_SMALL) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Get Memory Map failed!\r\n");
        return status;
    }

    memory_map_size += 2 * descriptor_size;

    EFI_MEMORY_DESCRIPTOR *memory_map = 0;

    status = SystemTable->BootServices->AllocatePool(EfiLoaderData, memory_map_size, (void**)&memory_map);
    status = SystemTable->BootServices->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);

    if (status != EFI_SUCCESS) {
        efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_RED " ERROR " EFI_COLOR_WHITE "> " "Failed to get memory map!\r\n");
        return status;
    }

    boot_info->memory_entry_count = 0;

    UINTN entry_count = memory_map_size / descriptor_size;
    uint32_t e820_count = 0;
    uint8_t *ptr = (uint8_t*)(UINTN)E820_MAP_ADDR;

    for (UINTN i = 0; i < entry_count; i++) {
        EFI_MEMORY_DESCRIPTOR *desc =
            (EFI_MEMORY_DESCRIPTOR *)(
                (UINT8 *)memory_map + i * descriptor_size
            );

        if (e820_count >= 128)
            break;

        uint32_t type;

        switch (desc->Type) {
            case EfiConventionalMemory:
                type = 1; // usable
                break;

            case EfiACPIReclaimMemory:
                type = 3;
                break;

            case EfiACPIMemoryNVS:
                type = 4;
                break;

            case EfiUnusableMemory:
                type = 5;
                break;

            default:
                type = 2; // reserved
                break;
        }

        *(uint64_t *)(ptr + 0) = desc->PhysicalStart;

        *(uint64_t *)(ptr + 8) = desc->NumberOfPages * 4096ULL;

        *(uint32_t *)(ptr + 16) = type;

        *(uint32_t *)(ptr + 20) = 0;

        ptr += 24;
        e820_count++;
    }

    *(uint16_t*)(UINTN)E820_COUNT_ADDR = e820_count;

    efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "Memory map loaded!\r\n");

    // efi_clear_screen(SystemTable);

    status = SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);

    if (status != EFI_SUCCESS) {
        return status;
    }

    // Kernel Jump
    
    KERNEL_ENTRY kernel_entry = (KERNEL_ENTRY)(UINT64)elf.e_entry;

    kernel_entry();

    /*
    UINT64 bss_start = ph.p_paddr + ph.p_filesz;
    UINT64 bss_size  = ph.p_memsz - ph.p_filesz;

    efi_print(SystemTable, "Before BSS\r\n");

    UINT64 *bss = (UINT64 *)(uintptr_t)bss_start;

    UINT64 qwords = bss_size / sizeof(UINT64);

    for (UINT64 i = 0; i < qwords; i++) {
        bss[i] = 0;
    }
    efi_print(SystemTable, "After BSS\r\n");

    efi_clear_screen(SystemTable);

    efi_print(
        SystemTable,
        EFI_COLOR_WHITE "Screen test after BSS\r\n"
    ); */

    // efi_print(SystemTable, EFI_COLOR_WHITE "<" EFI_COLOR_LIGHTBLUE " BOOTLOADER " EFI_COLOR_WHITE "> " "BSS initialized!\r\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }

    return EFI_SUCCESS;
}