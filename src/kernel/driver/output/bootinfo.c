#include <bootinfo.h>
#include <string.h>
#include <stdint.h>

BootInfo *boot_info = (BootInfo*)BOOT_INFO_ADDR;

static BootInfo boot_info_copy;

void boot_info_relocate(void) {
    memcpy(&boot_info_copy, boot_info, sizeof(BootInfo));
    boot_info = &boot_info_copy;
}