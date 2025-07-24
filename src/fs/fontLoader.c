/**
 * @file fontLoader.c
 * @brief Font Loading and Baking Implementation
 *
 * Handles loading of font files, baking glyphs into bitmap atlases, and
 * providing font metrics for text rendering in the kernel, using stb_truetype.
 */

#include <kstd/kstring.h>
#include <memory/kmemory.h>
#include <fs/fontLoader.h>
#include <kmath.h>
#include <stdint.h>

/* Map stb_truetype library functions to kernel implementations */
#define STBTT_malloc(x, y) AllocatePool(x)
#define STBTT_free(x, y) FreePool(x)
#define STBTT_strlen(x) kernel_strlen(x)
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

/* Include stb_truetype implementation */
#define STB_TRUETYPE_IMPLEMENTATION
#include <fs/stb_truetype.h>

/**
 * @brief Initializes a terminal font from a file
 * @param integratedTerminalFont Pointer to Font structure to initialize
 * @param fileName Path to font file to load
 * @param fontSize Desired rendering size in pixels
 * @param imageHandle EFI handle for accessing filesystem
 *
 * This function:
 * 1. Sets up basic font metrics based on requested size
 * 2. Uses EFI protocols to locate and load the font file
 * 3. Bakes the font into a bitmap atlas using stb_truetype
 * 4. Stores all glyph metrics for rendering
 */
void font_init(font_t* integratedTerminalFont, uint16_t* fileName, float fontSize, EFI_HANDLE imageHandle)
{
    /* Initialize basic font properties */
    integratedTerminalFont->font_size = fontSize;
    integratedTerminalFont->width = fontSize / 2;
    integratedTerminalFont->height = fontSize;

    /* Variables for font file loading */
    VOID* font_file_buffer;
    UINTN font_file_size;

    /* EFI protocols for filesystem access */
    EFI_LOADED_IMAGE_PROTOCOL* loadedImage = NULL;
    EFI_FILE_PROTOCOL* rootDir = NULL;

    /* Get loaded image protocol to locate font file */
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, imageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&loadedImage);

    /* Access filesystem protocol */
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FsProtocol;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FsProtocol);

    if (EFI_ERROR(status))
    {
        Print(u"ERROR: Failed to get filesystem protocol\n");
        return;
    }

    /* Open root directory */
    status = uefi_call_wrapper(FsProtocol->OpenVolume, 2, FsProtocol, &rootDir);
    if (EFI_ERROR(status))
    {
        Print(u"ERROR: Failed to open volume\n");
        return;
    }
    /* Open font file */
    EFI_FILE_PROTOCOL* file = 0;
    status = uefi_call_wrapper(rootDir->Open, 5, rootDir, &file, fileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(u"ERROR: Failed to open font file\n");
        return;
    }

    /* Get file information to determine size */
    EFI_FILE_INFO* fileInfo;
    UINTN infoSize = sizeof(EFI_FILE_INFO) + 128;
    fileInfo = (EFI_FILE_INFO*)AllocatePool(infoSize);
    status = uefi_call_wrapper(file->GetInfo, 4, file, &gEfiFileInfoGuid, &infoSize, fileInfo);
    if (EFI_ERROR(status))
    {
        FreePool(fileInfo);
        file->Close(file);
        Print(u"ERROR: Failed to get file info\n");
        return;
    }

    /* Allocate buffer and read font file */
    font_file_size = fileInfo->FileSize;
    font_file_buffer = AllocatePool(font_file_size);
    status = uefi_call_wrapper(file->Read, 3, file, &font_file_size, font_file_buffer);

    /* Clean up file handles */
    FreePool(fileInfo);
    file->Close(file);

    /* Bake font into atlas using stb_truetype */
    if (stbtt_BakeFontBitmap(font_file_buffer, 0, fontSize, (unsigned char*)integratedTerminalFont->atlas, ATLAS_W, ATLAS_H, FIRST_CHAR, NUM_CHARS, (stbtt_bakedchar*)integratedTerminalFont->cdata) <= 0)
    {
        return;
    }

    /* Free font file buffer */
    FreePool(font_file_buffer);
}