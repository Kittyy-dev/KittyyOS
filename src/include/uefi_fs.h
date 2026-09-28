#ifndef UEFI_FS_H
#define UEFI_FS_H

#include <efi.h>
#include <stdint.h>

EFI_STATUS efi_open_volume(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, EFI_FILE_PROTOCOL **Root);
EFI_STATUS efi_open_file(EFI_FILE_PROTOCOL *Root, CHAR16 *Path, EFI_FILE_PROTOCOL **File);

#endif