#include <efi.h>
#include <stdint.h>

void efi_print(EFI_SYSTEM_TABLE *SystemTable, const char *String);
void efi_clear_screen(EFI_SYSTEM_TABLE *SystemTable);
void efi_set_color(EFI_SYSTEM_TABLE *SystemTable, UINTN foreground, UINTN background);
void efi_printf(EFI_SYSTEM_TABLE *SystemTable, const char* fmt, ...);
void efi_print_hex(EFI_SYSTEM_TABLE *SystemTable, UINT64 value);