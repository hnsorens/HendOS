/**
 * @file fontLoader.h
 * @brief Font Loading and Management Interface
 *
 * Provides structures and functions for loading and managing bitmap fonts using stb_truetype.
 */

#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include <efi.h>
#include <efilib.h>
#include <kint.h>

/* Font atlas dimensions */
#define FIRST_CHAR 32 ///< First ASCII character to include in font atlas
#define NUM_CHARS 96  ///< Number of characters to include in font atlas
#define ATLAS_W 512   ///< Width of font atlas texture
#define ATLAS_H 512   ///< Height of font atlas texture

/**
 * @struct stbtt_bakedchar_t
 * @brief Character metrics for baked font glyphs
 *
 * Contains positioning and advance information for each character
 * in the font atlas.
 */
typedef struct stbtt_bakedchar_t
{
    unsigned short x0, y0, x1, y1; ///< Coordinates of glyph in the atlas
    float xoff, yoff;              ///< Glyph offset from drawing position
    float xadvance;                ///< Horizontal advance after this glyph
} stbtt_bakedchar_t;

/**
 * @struct Font
 * @brief Complete font representation
 *
 * Contains all data needed to render text using a baked font,
 * including the texture atlas and character metrics.
 */
typedef struct font_t
{
    uint8_t atlas[ATLAS_H][ATLAS_W];
    stbtt_bakedchar_t cdata[NUM_CHARS];
    uint32_t width;
    uint32_t height;
    float font_size;
} font_t;

/**
 * @struct Font
 * @brief Complete font representation
 *
 * Contains all data needed to render text using a baked font,
 * including the texture atlas and character metrics.
 */
void font_init(font_t* integratedTerminalFont, uint32_t* fileName, float fontSize, EFI_HANDLE imageHandle);

#endif /* FONT_LOADER_H */