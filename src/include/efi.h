#ifndef EFI_H
#define EFI_H

#include <stdint.h>

typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef UINT64 UINTN;

typedef UINT16 CHAR16;

typedef UINT64 EFI_STATUS;

typedef void *EFI_HANDLE;

typedef UINT8 BOOLEAN;

#define EFI_SUCCESS 0

#define EFI_BLACK        0x00
#define EFI_BLUE         0x01
#define EFI_GREEN        0x02
#define EFI_CYAN         0x03
#define EFI_RED          0x04
#define EFI_MAGENTA      0x05
#define EFI_BROWN        0x06
#define EFI_LIGHTGRAY    0x07

#define EFI_DARKGRAY     0x08
#define EFI_LIGHTBLUE    0x09
#define EFI_LIGHTGREEN   0x0A
#define EFI_LIGHTCYAN    0x0B
#define EFI_LIGHTRED     0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW       0x0E
#define EFI_WHITE        0x0F

#define AllocateAnyPages   0
#define AllocateMaxAddress 1
#define AllocateAddress    2

#define EfiLoaderData      4

#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5B1B31A1, 0x9562, 0x11D2, \
      { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x964E5B22, 0x6459, 0x11D2, \
      { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042A9DE, 0x23DC, 0x4A38, \
      { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A } }

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_EXIT_BOOT_SERVICES)(
    EFI_HANDLE ImageHandle,
    UINTN MapKey
);

typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(
    EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    UINT32 PixelInformation[4];
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINT64 FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber,
    UINTN *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber
);

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    UINT32 Type;
    UINT32 Padding;
    UINT64 PhysicalStart;
    UINT64 VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef EFI_STATUS (*EFI_ALLOCATE_POOL)(
    UINT32 PoolType,
    UINTN Size,
    void **Buffer
);

typedef EFI_STATUS (*EFI_ALLOCATE_PAGES)(
    UINT32 Type,
    UINT32 MemoryType,
    UINTN Pages,
    UINT64 *Memory
);

typedef EFI_STATUS (*EFI_GET_MEMORY_MAP)(
    UINTN *MemoryMapSize,
    void *MemoryMap,
    UINTN *MapKey,
    UINTN *DescriptorSize,
    UINT32 *DescriptorVersion
);

typedef EFI_STATUS (*EFI_EXIT_BOOT_SERVICES)(
    EFI_HANDLE ImageHandle,
    UINTN MapKey
);

typedef EFI_STATUS (*EFI_TEXT_RESET)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (*EFI_TEXT_STRING)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16 *String
);

typedef EFI_STATUS (*EFI_TEXT_TEST_STRING)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16 *String
);

typedef EFI_STATUS (*EFI_TEXT_QUERY_MODE)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber,
    UINTN *Columns,
    UINTN *Rows
);

typedef EFI_STATUS (*EFI_TEXT_SET_MODE)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber
);

typedef EFI_STATUS (*EFI_TEXT_SET_ATTRIBUTE)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Attribute
);

typedef EFI_STATUS (*EFI_TEXT_CLEAR_SCREEN)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef EFI_STATUS (*EFI_TEXT_SET_CURSOR_POSITION)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
);

typedef EFI_STATUS (*EFI_TEXT_ENABLE_CURSOR)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    BOOLEAN Visible
);

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {

    EFI_TEXT_RESET Reset;

    EFI_TEXT_STRING OutputString;

    EFI_TEXT_TEST_STRING TestString;

    EFI_TEXT_QUERY_MODE QueryMode;

    EFI_TEXT_SET_MODE SetMode;

    EFI_TEXT_SET_ATTRIBUTE SetAttribute;

    EFI_TEXT_CLEAR_SCREEN ClearScreen;

    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;

    EFI_TEXT_ENABLE_CURSOR EnableCursor;

    void *Mode;

} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct EFI_TABLE_HEADER {

    UINT64 Signature;

    UINT32 Revision;

    UINT32 HeaderSize;

    UINT32 CRC32;

    UINT32 Reserved;

} EFI_TABLE_HEADER;

typedef EFI_STATUS (*EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle,
    EFI_GUID *Protocol,
    void **Interface
);

typedef struct EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;

    void *RaiseTPL;
    void *RestoreTPL;

    EFI_ALLOCATE_PAGES AllocatePages;
    void *FreePages;

    EFI_GET_MEMORY_MAP GetMemoryMap;

    EFI_ALLOCATE_POOL AllocatePool;
    void *FreePool;

    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;

    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;

    EFI_HANDLE_PROTOCOL HandleProtocol;

    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;

    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;

    EFI_EXIT_BOOT_SERVICES ExitBootServices;

    void *GetNextMonotonicCount;
    void *Stall;
    void *SetWatchdogTimer;

    void *ConnectController;
    void *DisconnectController;

    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;

    void *ProtocolsPerHandle;
    void *LocateHandleBuffer;

    EFI_LOCATE_PROTOCOL LocateProtocol;

    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;

    void *CalculateCrc32;
    void *CopyMem;
    void *SetMem;

    void *CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct EFI_SYSTEM_TABLE {

    EFI_TABLE_HEADER Hdr;

    CHAR16 *FirmwareVendor;

    UINT32 FirmwareRevision;

    EFI_HANDLE ConsoleInHandle;

    void *ConIn;

    EFI_HANDLE ConsoleOutHandle;

    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;

    EFI_HANDLE StandardErrorHandle;

    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;

    void *RuntimeServices;

    EFI_BOOT_SERVICES *BootServices;

} EFI_SYSTEM_TABLE;

typedef struct {

    UINT32 Revision;

    EFI_HANDLE ParentHandle;

    EFI_SYSTEM_TABLE *SystemTable;

    EFI_HANDLE DeviceHandle;

    void *FilePath;

    void *Reserved;

    UINT32 LoadOptionsSize;

    void *LoadOptions;

    void *ImageBase;

    UINT64 ImageSize;

    UINT32 ImageCodeType;

    UINT32 ImageDataType;

    void *Unload;

} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef EFI_STATUS (*EFI_OPEN_VOLUME)(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    void **Root
);

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {

    UINT64 Revision;

    EFI_OPEN_VOLUME OpenVolume;

};

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (*EFI_FILE_OPEN)(
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes
);

typedef EFI_STATUS (*EFI_FILE_CLOSE)(
    EFI_FILE_PROTOCOL *This
);

typedef EFI_STATUS (*EFI_FILE_DELETE)(
    EFI_FILE_PROTOCOL *This
);

typedef EFI_STATUS (*EFI_FILE_READ)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);

typedef EFI_STATUS (*EFI_FILE_WRITE)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);

typedef EFI_STATUS (*EFI_FILE_GET_POSITION)(
    EFI_FILE_PROTOCOL *This,
    UINT64 *Position
);

typedef EFI_STATUS (*EFI_FILE_SET_POSITION)(
    EFI_FILE_PROTOCOL *This,
    UINT64 Position
);

struct EFI_FILE_PROTOCOL {

    UINT64 Revision;

    EFI_FILE_OPEN Open;

    EFI_FILE_CLOSE Close;

    EFI_FILE_DELETE Delete;

    EFI_FILE_READ Read;

    EFI_FILE_WRITE Write;

    EFI_FILE_GET_POSITION GetPosition;

    EFI_FILE_SET_POSITION SetPosition;

};

#define EFI_FILE_MODE_READ 0x0000000000000001ULL

#define EFIAPI


#endif