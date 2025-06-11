#include <drivers/fbcon.h>
#include <fs/fontLoader.h>
#include <fs/stb_truetype.h>
#include <kernel/device.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <misc/debug.h>

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
            // if (px >= 0 && px < GRAPHICS_CONTEXT->screen_width && py >= 0 &&
            //     py < GRAPHICS_CONTEXT->screen_height && tx >= 0 && tx < ATLAS_W && ty >= 0 &&
            //     ty < ATLAS_H)
            // {
            uint8_t value = INTEGRATED_FONT->atlas[ty][tx];
            color = (color & 0x00FFFFFF) | (value << 24);
            ((uint32_t*)FRAMEBUFFER_START)[py * GRAPHICS_CONTEXT->screen_width + px] =
                BLEND_PIXELS(0, color);
            // }
        }
    }
}

void fbcon_init()
{
    /* Create Device */
    FBCON->dev_id = dev_create("fbcon", 0);

    /* Registers Callbacks */
    dev_register_kernel_callback(FBCON->dev_id, 0, fbcon_render); /* ID 0 is render */
    dev_register_kernel_callback(FBCON->dev_id, 1, fbcon_scroll); /* ID 1 is scroll */

    /* Move this to somewhere else */
    for (int i = 0; i < 1080 * 1920; i++)
    {
        ((uint32_t*)FRAMEBUFFER_START)[i] = 0;
    }
}

void fbcon_render(uint64_t character, uint64_t position)
{
    /* Gets position of the new character */
    uint32_t pos_y = (uint32_t)(position & 0xFFFFFFFF) + 1;
    uint32_t pos_x = (uint32_t)(position >> 32);

    fbcon_draw_character(character, pos_x * 14, pos_y * 20, 0xFFFFFFFF);
}

void fbcon_scroll(uint64_t amount, uint64_t _unused) {}