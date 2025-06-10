/* fontLoader.c */
#include <fs/fontLoader.h>

#include <kmath.h>
#define STBTT_malloc(x, y) AllocatePool(x)
#define STBTT_free(x, y) FreePool(x)
#define STBTT_strlen(x) StrLen(x)
#define STBTT_memcpy(x, y, z) kmemcpy(x, y, z)
#define STBTT_memset(x, y, z) kmemset(x, y, z)
#define STBTT_ifloor(x) ((int)floor(x))
#define STBTT_iceil(x) ((int)ceiling(x))
#define STBTT_sqrt(x) sqrt(x)
#define STBTT_pow(x, y) pow(x, y)
#define STBTT_fmod(x, y) fmod(x, y)
#define STBTT_cos(x) cos(x)
#define STBTT_acos(x) acos(x)
#define STBTT_fabs(x) fabs(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include <fs/stb_truetype.h>

void KERNEL_InitTerminalFont(Font* integratedTerminalFont,
                             uint32_t* fileName,
                             float fontSize,
                             EFI_HANDLE imageHandle)
{
    integratedTerminalFont->font_size = fontSize;
    integratedTerminalFont->width = fontSize / 2;
    integratedTerminalFont->height = fontSize;

    VOID* font_file_buffer;
    UINTN font_file_size;

    EFI_LOADED_IMAGE_PROTOCOL* loadedImage = NULL;
    EFI_FILE_PROTOCOL* rootDir = NULL;

    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, imageHandle,
                                          &gEfiLoadedImageProtocolGuid, (VOID**)&loadedImage);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FsProtocol;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, loadedImage->DeviceHandle,
                               &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FsProtocol);

    if (EFI_ERROR(status))
    {
        Print(L"ERROR\n");
    }

    status = uefi_call_wrapper(FsProtocol->OpenVolume, 2, FsProtocol, &rootDir);
    if (EFI_ERROR(status))
    {
        Print(L"ERROR\n");
    }

    EFI_FILE_PROTOCOL* file = 0;

    status = uefi_call_wrapper(rootDir->Open, 5, rootDir, &file, fileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(L"ERROR\n");
    }

    EFI_FILE_INFO* fileInfo;
    UINTN infoSize = sizeof(EFI_FILE_INFO) + 128;
    fileInfo = (EFI_FILE_INFO*)AllocatePool(infoSize);
    status = uefi_call_wrapper(file->GetInfo, 4, file, &gEfiFileInfoGuid, &infoSize, fileInfo);
    if (EFI_ERROR(status))
    {
        FreePool(fileInfo);
        file->Close(file);
        Print(L"ERROR\n");
    }

    font_file_size = fileInfo->FileSize;
    font_file_buffer = AllocatePool(font_file_size);

    status = uefi_call_wrapper(file->Read, 3, file, &font_file_size, font_file_buffer);

    FreePool(fileInfo);
    file->Close(file);

    if (stbtt_BakeFontBitmap(
            font_file_buffer, 0, fontSize, (unsigned char*)integratedTerminalFont->atlas, ATLAS_W,
            ATLAS_H, FIRST_CHAR, NUM_CHARS, (stbtt_bakedchar*)integratedTerminalFont->cdata) <= 0)
    {
        return;
    }

    FreePool(font_file_buffer);
}