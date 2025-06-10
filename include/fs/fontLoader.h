#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include <efi.h>
#include <efilib.h>
#include <kint.h>

#define FIRST_CHAR 32
#define NUM_CHARS 96
#define ATLAS_W 512
#define ATLAS_H 512

typedef struct
{
    unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
    float xoff, yoff, xadvance;
} KERNEL_stbtt_bakedchar;

typedef struct
{
    uint8_t atlas[ATLAS_H][ATLAS_W];
    KERNEL_stbtt_bakedchar cdata[NUM_CHARS];
    uint32_t width;
    uint32_t height;
    float font_size;
} Font;

void KERNEL_InitTerminalFont(Font* integratedTerminalFont,
                             uint32_t* fileName,
                             float fontSize,
                             EFI_HANDLE imageHandle);

#endif