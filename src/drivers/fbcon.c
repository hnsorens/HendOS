/**
 * @file fbcon.c
 * @brief Framebuffer Console Driver Implementation
 *
 * Provides functions for text rendering, scrolling, and device file integration
 * for the kernel's framebuffer console.
 */
#include <drivers/fbcon.h>
#include <fs/fontLoader.h>
#include <fs/stb_truetype.h>
#include <kernel/device.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <misc/debug.h>

#define CHARACTER_WIDTH 12
#define CHARACTER_HEIGHT 22

static void fbcon_draw_character(uint16_t ch, float x, float y, int color)
{
    if (ch < FIRST_CHAR || ch >= FIRST_CHAR + NUM_CHARS)
        return;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(INTEGRATED_FONT->cdata, ATLAS_W, ATLAS_H, ch - FIRST_CHAR, &x, &y, &q, 1);

    for (int py = (int)q.y0; py < (int)q.y1; ++py)
    {
        for (int px = (int)q.x0; px < (int)q.x1; ++px)
        {
            int tx = (int)(q.s0 * ATLAS_W + (px - q.x0));
            int ty = (int)(q.t0 * ATLAS_H + (py - q.y0));
            uint8_t value = INTEGRATED_FONT->atlas[ty][tx];
            color = (color & 0x00FFFFFF) | (value << 24);
            ((uint32_t*)FRAMEBUFFER_START)[py * GRAPHICS_CONTEXT->screen_width + px] = BLEND_PIXELS(0, color);
        }
    }
}

void fbcon_init()
{
    /* Create device file */
    vfs_entry_t* device_file = vfs_create_entry(*DEV, "fbcon", EXT2_FT_CHRDEV);

    device_file->ops[4] = fbcon_render;
    device_file->ops[5] = fbcon_scroll;

    FBCON->fbcon = fdm_open_file(device_file);

    /* Move this to somewhere else */
    for (int i = 0; i < 1080 * 1920; i++)
    {
        ((uint32_t*)FRAMEBUFFER_START)[i] = 0;
    }
}

void fbcon_render(file_descriptor_t* open_file, uint64_t character, uint64_t position)
{
    /* Gets position of the new character */
    uint32_t pos_y = (uint32_t)(position & 0xFFFFFFFF) + 1;
    uint32_t pos_x = (uint32_t)(position >> 32);

    for (int px_y = 0; px_y < CHARACTER_HEIGHT; px_y++)
    {
        for (int px_x = 0; px_x < CHARACTER_WIDTH; px_x++)
        {
            ((uint32_t*)FRAMEBUFFER_START)[(pos_x * CHARACTER_WIDTH + px_x) + (pos_y * CHARACTER_HEIGHT - px_y) * GRAPHICS_CONTEXT->screen_width] = 0xFF000000;
        }
    }

    fbcon_draw_character(character, pos_x * CHARACTER_WIDTH, pos_y * CHARACTER_HEIGHT - 5, 0xFFFFFFFF);
}

void fbcon_scroll(file_descriptor_t* open_file, uint64_t amount, uint64_t _unused)
{
    uint64_t offset = GRAPHICS_CONTEXT->screen_width * CHARACTER_HEIGHT;
    for (int i = offset; i < 1920 * 1080; i++)
    {
        ((uint32_t*)FRAMEBUFFER_START)[i - offset] = ((uint32_t*)FRAMEBUFFER_START)[i];
    }
}