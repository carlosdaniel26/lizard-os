#pragma once
/*
 * Minimal hand-rolled UEFI declarations - just what boot/main.c uses.
 * All firmware entry points use the MS x64 calling convention.
 */

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef unsigned long long UINTN;
typedef long long          INTN;
typedef UINTN              EFI_STATUS;
typedef void              *EFI_HANDLE;
typedef u16                CHAR16;
typedef u64                EFI_PHYSICAL_ADDRESS;
typedef u64                EFI_VIRTUAL_ADDRESS;

#define EFIAPI __attribute__((ms_abi))
#define IN
#define OUT
#define OPTIONAL

#define EFI_SUCCESS            0ULL
#define EFI_LOAD_ERROR        (0x8000000000000000ULL | 1)
#define EFI_INVALID_PARAMETER (0x8000000000000000ULL | 2)
#define EFI_UNSUPPORTED       (0x8000000000000000ULL | 3)
#define EFI_BUFFER_TOO_SMALL  (0x8000000000000000ULL | 5)
#define EFI_NOT_FOUND         (0x8000000000000000ULL | 14)
#define EFI_ERROR(s)          (((INTN)(s)) < 0)

#define EFI_PAGE_SIZE 4096

typedef struct {
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8  Data4[8];
} EFI_GUID;

/* enum EFI_ALLOCATE_TYPE */
#define AllocateAnyPages    0
#define AllocateMaxAddress  1
#define AllocateAddress     2

/* enum EFI_MEMORY_TYPE */
enum {
    EfiReservedMemoryType = 0,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType,
};

typedef struct {
    u32 Type;
    u32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS  VirtualStart;
    u64 NumberOfPages;
    u64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Attribute);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void                  *Reset;
    EFI_TEXT_STRING        OutputString;
    void                  *TestString;
    void                  *QueryMode;
    void                  *SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN  ClearScreen;
    void                  *SetCursorPosition;
    void                  *EnableCursor;
    void                  *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    char                             _hdr[24];
    void                            *RaiseTPL;
    void                            *RestoreTPL;
    EFI_STATUS (EFIAPI *AllocatePages)(UINTN Type, UINTN MemoryType, UINTN Pages,
                                       EFI_PHYSICAL_ADDRESS *Memory);
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
    EFI_STATUS (EFIAPI *GetMemoryMap)(UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap,
                                      UINTN *MapKey, UINTN *DescriptorSize, u32 *DescriptorVersion);
    EFI_STATUS (EFIAPI *AllocatePool)(UINTN PoolType, UINTN Size, void **Buffer);
    EFI_STATUS (EFIAPI *FreePool)(void *Buffer);
    void                            *CreateEvent;
    void                            *SetTimer;
    void                            *WaitForEvent;
    void                            *SignalEvent;
    void                            *CloseEvent;
    void                            *CheckEvent;
    void                            *InstallProtocolInterface;
    void                            *ReinstallProtocolInterface;
    void                            *UninstallProtocolInterface;
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
    void                            *Reserved;
    void                            *RegisterProtocolNotify;
    void                            *LocateHandle;
    void                            *LocateDevicePath;
    void                            *InstallConfigurationTable;
    void                            *LoadImage;
    void                            *StartImage;
    void                            *Exit;
    void                            *UnloadImage;
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);
    void                            *GetNextMonotonicCount;
    EFI_STATUS (EFIAPI *Stall)(UINTN Microseconds);
    void                            *SetWatchdogTimer;
    void                            *ConnectController;
    void                            *DisconnectController;
    void                            *OpenProtocol;
    void                            *CloseProtocol;
    void                            *OpenProtocolInformation;
    void                            *ProtocolsPerHandle;
    void                            *LocateHandleBuffer;
    EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
    /* ... rest unused ... */
} EFI_BOOT_SERVICES;

typedef struct {
    char   _hdr[24];
    CHAR16 *FirmwareVendor;
    u32     FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void   *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    void   *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN   NumberOfTableEntries;
    void   *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* --- Loaded Image --- */
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1,0x9562,0x11d2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}

typedef struct {
    u32        Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void      *FilePath;
    void      *Reserved;
    u32        LoadOptionsSize;
    void      *LoadOptions;
    void      *ImageBase;
    u64        ImageSize;
    UINTN      ImageCodeType;
    UINTN      ImageDataType;
    void      *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

/* --- Simple File System / File --- */
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964E5B22,0x6459,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_FILE_INFO_GUID \
    {0x09576E92,0x6D3F,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}

#define EFI_FILE_MODE_READ  0x0000000000000001ULL

struct _EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(struct _EFI_FILE_PROTOCOL *This,
    struct _EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, u64 OpenMode, u64 Attributes);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(struct _EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(struct _EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(struct _EFI_FILE_PROTOCOL *This, u64 Position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(struct _EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType,
    UINTN *BufferSize, void *Buffer);

typedef struct _EFI_FILE_PROTOCOL {
    u64                   Revision;
    EFI_FILE_OPEN         Open;
    EFI_FILE_CLOSE        Close;
    void                 *Delete;
    EFI_FILE_READ         Read;
    void                 *Write;
    EFI_FILE_SET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO     GetInfo;
    void                 *SetInfo;
    void                 *Flush;
} EFI_FILE_PROTOCOL;

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    u64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
                                    EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    u64    Size;
    u64    FileSize;
    u64    PhysicalSize;
    char   CreateTime[16];
    char   LastAccessTime[16];
    char   ModificationTime[16];
    u64    Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

/* --- Graphics Output Protocol --- */
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042A9DE,0x23DC,0x4A38,{0x96,0xFB,0x7A,0xDE,0xD0,0x80,0x51,0x6A}}

enum {
    PixelRedGreenBlueReserved8BitPerColor = 0,
    PixelBlueGreenRedReserved8BitPerColor = 1,
    PixelBitMask                          = 2,
    PixelBltOnly                          = 3,
};

typedef struct {
    u32 RedMask, GreenMask, BlueMask, ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    u32               Version;
    u32               HorizontalResolution;
    u32               VerticalResolution;
    u32               PixelFormat;        /* enum above */
    EFI_PIXEL_BITMASK PixelInformation;
    u32               PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    u32                                   MaxMode;
    u32                                   Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                                 SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                  FrameBufferBase;
    UINTN                                 FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void *QueryMode;
    EFI_STATUS (EFIAPI *SetMode)(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This, u32 ModeNumber);
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;
