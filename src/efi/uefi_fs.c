#include <stdint.h>
#include <efi.h>
#include <uefi_fs.h>
#include <gopefi.h>

static EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID simple_fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

EFI_STATUS efi_open_volume(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, EFI_FILE_PROTOCOL **Root) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = 0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = 0;

    status = SystemTable->BootServices->HandleProtocol(ImageHandle, &loaded_image_guid, (void**)&loaded_image);

    if (status != EFI_SUCCESS) {
        return status;
    }

    status = SystemTable->BootServices->HandleProtocol(loaded_image->DeviceHandle, &simple_fs_guid, (void**)&fs);

    if (status != EFI_SUCCESS) {
        return status;
    }

    status = fs->OpenVolume(fs, (void**)Root);

    return status;
}

EFI_STATUS efi_open_file(EFI_FILE_PROTOCOL *Root, CHAR16 *Path, EFI_FILE_PROTOCOL **File) {
    return Root->Open(Root, File, Path, EFI_FILE_MODE_READ, 0);
}