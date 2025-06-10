/* bootServices.h */
#ifndef BOOT_SERVICES_H
#define BOOT_SERVICES_H

#include <efi.h>
#include <efilib.h>

typedef struct preboot_info_t
{
    /* Framebuffer Information */
    UINT32* framebuffer;
    UINT32 screen_width;
    UINT32 screen_height;

    /* Preboot Memory Map Information */
    UINTN MemoryMapSize;
    EFI_MEMORY_DESCRIPTOR* MemoryMap;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    uint64_t framebuffer_size;
} preboot_info_t;

EFI_STATUS KERNEL_ExitBootService(preboot_info_t* preboot_info,
                                  EFI_HANDLE ImageHandle,
                                  EFI_SYSTEM_TABLE* SystemTable);

typedef struct
{
    UINT64 base;
    UINT64 size;
} MemoryRegion;

EFI_STATUS KERNEL_FindMemoryRegions(struct preboot_info_t* preboot_info,
                                    MemoryRegion* regions,
                                    size_t regions_count);

#endif