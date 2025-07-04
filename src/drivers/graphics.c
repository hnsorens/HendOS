/* graphics.c */

#include <drivers/graphics.h>
#include <fs/fontLoader.h>

#include <boot/bootServices.h>
#include <fs/stb_truetype.h>
#include <kmath.h>
#include <memory/kglobals.h>
#include <memory/memoryMap.h>
#include <memory/pmm.h>

void GRAPHICS_InitGraphics()
{
    preboot_info_t* info = PREBOOT_INFO;

    GRAPHICS_CONTEXT->screen_width = info->screen_width;
    GRAPHICS_CONTEXT->screen_height = info->screen_height;
    GRAPHICS_CONTEXT->framebuffer = FRAMEBUFFER_START;
    GRAPHICS_CONTEXT->back_buffer = FRAMEBUFFER_START;
    GRAPHICS_CONTEXT->top_buffer = kmalloc(GRAPHICS_CONTEXT->screen_width * GRAPHICS_CONTEXT->screen_height * sizeof(uint32_t));
}

void GRAPHICS_CleanupGraphics() {}

void GRAPHICS_Clear(KERNEL_color color) {}

void GRAPHICS_DrawPixel(Layer* layer, int32_t x, int32_t y, int32_t color)
{
    layer->pixels[y * layer->width + x] = color;
}

void GRAPHICS_SafeDrawPixel(Layer* layer, int32_t x, int32_t y, KERNEL_color color)
{
    if (x >= 0 && x < (int32_t)layer->width && y >= 0 && y < (int32_t)layer->width)
    {
        GRAPHICS_DrawPixel(layer, x, y, color);
    }
}

void GRAPHICS_DrawLine(Layer* layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, KERNEL_color color)
{
    int32_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int32_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int32_t err = dx + dy, e2;

    while (1)
    {
        GRAPHICS_SafeDrawPixel(layer, x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void GRAPHICS_DrawRect(Layer* layer, int32_t x, int32_t y, int32_t w, int32_t h, KERNEL_color color)
{
    GRAPHICS_DrawLine(layer, x, y, x + w, y, color);
    GRAPHICS_DrawLine(layer, x + w, y, x + w, y + h, color);
    GRAPHICS_DrawLine(layer, x + w, y + h, x, y + h, color);
    GRAPHICS_DrawLine(layer, x, y + h, x, y, color);
}

void GRAPHICS_FillRect(Layer* layer, int32_t x, int32_t y, int32_t w, int32_t h, KERNEL_color color)
{
    int32_t x_end = min(x + w, (int32_t)layer->width);
    int32_t y_end = min(y + h, (int32_t)layer->height);
    x = max(x, 0);
    y = max(y, 0);

    for (int32_t cy = y; cy < y_end; ++cy)
    {
        uint32_t row = cy * layer->width;
        for (int32_t cx = x; cx < x_end; ++cx)
        {
            layer->pixels[row + cx] = color;
        }
    }
}

void GRAPHICS_DrawCircle(Layer* layer, int32_t cx, int32_t cy, uint32_t r, uint32_t line_thickness, KERNEL_color color)
{
    int32_t x_end = min(cx + r + line_thickness, (int32_t)layer->width);
    int32_t y_end = min(cy + r + line_thickness, (int32_t)layer->height);
    int32_t x = max(cx - r - line_thickness, 0);
    int32_t y = max(cy - r - line_thickness, 0);

    uint32_t squared_distance = r * r;

    for (int32_t y_pos = y; y_pos < y_end; ++y_pos)
    {
        uint32_t row = cy * layer->width;
        for (int32_t x_pos = x; x_pos < x_end; ++x_pos)
        {
            int32_t distance = abs(cx - x_pos) * abs(cy - y_pos);
            if (distance > squared_distance - line_thickness && distance < squared_distance + line_thickness)
            {
                layer->pixels[row + cx] = color;
            }
        }
    }
}

void GRAPHICS_FillCircle(Layer* layer, int32_t cx, int32_t cy, uint32_t r, KERNEL_color color)
{
    int32_t x_start = max(cx - (int32_t)r, 0);
    int32_t x_end = min(cx + (int32_t)r, (int32_t)layer->width);
    int32_t y_start = max(cy - (int32_t)r, 0);
    int32_t y_end = min(cy + (int32_t)r, (int32_t)layer->height);

    uint32_t r_squared = r * r;

    for (int32_t y_pos = y_start; y_pos < y_end; ++y_pos)
    {
        for (int32_t x_pos = x_start; x_pos < x_end; ++x_pos)
        {
            int32_t dx = x_pos - cx;
            int32_t dy = y_pos - cy;
            if ((dx * dx + dy * dy) <= (int32_t)r_squared)
            {
                layer->pixels[y_pos * layer->width + x_pos] = color;
            }
        }
    }
}

Layer* GRAPHICS_CreateLayer(void* layer_memory, void* pixels_memory, uint32_t w, uint32_t h)
{
    Layer* layer = layer_memory;

    layer->width = w;
    layer->height = h;
    layer->pixels = pixels_memory; // kmalloc(w * h * sizeof(uint32_t));
    layer->scale_x = 1.0f;
    layer->scale_y = 1.0f;
    layer->pos_x = 0;
    layer->pos_y = 0;
    layer->dirty_pos_x = 0;
    layer->dirty_pos_y = 0;
    layer->visible = true;
    layer->layer = 0;

    if (!layer->pixels)
    {
        kfree(layer);
        return 0;
    }

    for (uint32_t i = 0; i < w * h; i++)
    {
        layer->pixels[i] = COLOR_TRANSPARENT;
    }

    return layer;
}

void GRAPHICS_DestroyLayer(Layer* layer)
{
    if (layer)
    {
        if (layer->pixels)
            kfree(layer->pixels);
        kfree(layer);
    }
}

void GRAPHICS_ClearLayer(Layer* layer, KERNEL_color color)
{
    for (uint32_t i = 0; i < layer->width * layer->height; i++)
    {
        layer->pixels = color;
    }
}

void GRAPHICS_ApplyLayerOverride(Layer* layer, uint32_t index)
{
    layer->layer = index;
    GRAPHICS_LAYERS[index] = layer;

    layer->dirty_pos_x = layer->pos_x;
    layer->dirty_pos_y = layer->pos_y;
    layer->dirty_scale_x = layer->scale_x;
    layer->dirty_scale_y = layer->scale_y;

    for (int32_t cy = 0; cy < layer->height; ++cy)
    {
        uint32_t row = cy * layer->width;
        uint32_t context_row = (cy + layer->pos_y) * GRAPHICS_CONTEXT->screen_width;
        for (int32_t cx = 0; cx < layer->width; ++cx)
        {
            if (ALPHA_ARGB(layer->pixels[row + cx]) != 0 && GRAPHICS_CONTEXT->top_buffer[context_row + layer->pos_x + cx] <= index)
            {
                GRAPHICS_CONTEXT->back_buffer[context_row + layer->pos_x + cx] = layer->pixels[row + cx];
                GRAPHICS_CONTEXT->top_buffer[context_row + layer->pos_x + cx] = index;
            }
        }
    }
}

void GRAPHICS_ApplyLayer(Layer* layer)
{
    if (!layer)
        return;

    GRAPHICS_LAYERS[(*GRAPHICS_LAYER_COUNT)++] = layer;
    layer->layer = (*GRAPHICS_LAYER_COUNT) - 1;

    layer->dirty_pos_x = layer->pos_x;
    layer->dirty_pos_y = layer->pos_y;
    layer->dirty_scale_x = layer->scale_x;
    layer->dirty_scale_y = layer->scale_y;

    for (uint32_t cy = 0; cy < layer->height; ++cy)
    {

        uint32_t row = cy * layer->width;
        uint32_t context_row = (cy + layer->pos_y) * GRAPHICS_CONTEXT->screen_width;
        for (uint32_t cx = 0; cx < layer->width; ++cx)
        {
            if (ALPHA_ARGB(layer->pixels[row + cx]) != 0)
            {
                GRAPHICS_CONTEXT->back_buffer[(cy * GRAPHICS_CONTEXT->screen_width) + cx] = layer->pixels[row + cx];
                GRAPHICS_CONTEXT->top_buffer[context_row + layer->pos_x + cx] = (*GRAPHICS_LAYER_COUNT) - 1;
            }
        }
    }
}

bool GRAPHICS_IsTransparent(KERNEL_color color)
{
    return ALPHA_ARGB(color) == 0;
}

void GRAPHICS_RemoveLayer(Layer* layer)
{
    if (!layer)
        return;
    uint32_t layerIndex = layer->layer;
    for (int cy = 0; cy < layer->height; cy++)
    {
        for (int cx = 0; cx < layer->width; cx++)
        {
            uint32_t index = (cx + layer->dirty_pos_x) + (cy + layer->dirty_pos_y) * GRAPHICS_CONTEXT->screen_width;
            if (GRAPHICS_CONTEXT->top_buffer[index] == layerIndex)
            {
                for (uint32_t i = layerIndex - 1; i >= 0; --i)
                {
                    i = 0;
                    Layer* below = GRAPHICS_LAYERS[0];
                    int local_x = cx - below->dirty_pos_x + layer->dirty_pos_x;
                    int local_y = cy - below->dirty_pos_y + layer->dirty_pos_y;
                    if (local_x >= 0 && local_x < below->width && local_y >= 0 && local_y < below->height)
                    {
                        uint32_t px = below->pixels[local_x + local_y * below->width];
                        if (!GRAPHICS_IsTransparent(px))
                        {
                            GRAPHICS_CONTEXT->top_buffer[index] = i;
                            GRAPHICS_CONTEXT->back_buffer[index] = px;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void GRAPHICS_UpdateLayer(Layer* layer)
{

    if (!layer)
        return;
    uint32_t layerIndex = layer->layer;
    int oldX = layer->dirty_pos_x;
    int oldY = layer->dirty_pos_y;
    int dx = layer->pos_x - layer->dirty_pos_x;
    int dy = layer->pos_y - layer->dirty_pos_y;
    int framebufferWidth = GRAPHICS_CONTEXT->screen_width;
    int framebufferHeight = GRAPHICS_CONTEXT->screen_height;

    if (dx == 0 && dy == 0)
    {
        // Position unchanged; only update visible pixels
        for (int y = 0; y < layer->height; ++y)
        {
            for (int x = 0; x < layer->width; ++x)
            {
                int fx = oldX + x;
                int fy = oldY + y;
                // if (fx < 0 || fy < 0 || fx >= GRAPHICS_CONTEXT->screen_width || fy >=
                // GRAPHICS_CONTEXT->screen_height) continue;
                int index = fx + fy * GRAPHICS_CONTEXT->screen_width;
                if (GRAPHICS_CONTEXT->top_buffer[index] <= layerIndex && !GRAPHICS_IsTransparent(layer->pixels[x + y * layer->width]))
                {
                    GRAPHICS_CONTEXT->top_buffer[index] = layerIndex;
                    GRAPHICS_CONTEXT->back_buffer[index] = layer->pixels[x + y * layer->width];
                }
            }
        }
        return;
    }

    // Handle movement
    for (int y = 0; y < layer->height; ++y)
    {
        for (int x = 0; x < layer->width; ++x)
        {
            int old_fx = oldX + x;
            int old_fy = oldY + y;
            if (old_fx < 0 || old_fy < 0 || old_fx >= framebufferWidth || old_fy >= framebufferHeight)
                continue;

            int index = old_fx + old_fy * framebufferWidth;

            // Check if it still intersects new position
            int stillIntersects = old_fx >= layer->pos_x && old_fx < layer->pos_x + layer->width && old_fy >= layer->pos_y && old_fy < layer->pos_y + layer->height;

            if (stillIntersects)
            {
                // If top layer is this one, update color if not transparent
                if (GRAPHICS_CONTEXT->top_buffer[index] <= layerIndex && !GRAPHICS_IsTransparent(layer->pixels[(x - dx) + (y - dy) * layer->width]))
                {
                    GRAPHICS_CONTEXT->top_buffer[index] = layerIndex;
                    GRAPHICS_CONTEXT->back_buffer[index] = layer->pixels[(x - dx) + (y - dy) * layer->width];
                }
                else if (GRAPHICS_IsTransparent(layer->pixels[(x - dx) + (y - dy) * layer->width]))
                {
                    for (int i = layerIndex - 1; i >= 0; --i)
                    {
                        Layer* below = GRAPHICS_LAYERS[i];
                        int local_x = old_fx - below->dirty_pos_x;
                        int local_y = old_fy - below->dirty_pos_y;
                        if (local_x >= 0 && local_x < below->width && local_y >= 0 && local_y < below->height)
                        {
                            uint32_t px = below->pixels[local_x + local_y * below->width];
                            if (!GRAPHICS_IsTransparent(px))
                            {
                                GRAPHICS_CONTEXT->top_buffer[index] = i;
                                GRAPHICS_CONTEXT->back_buffer[index] = px;
                                break;
                            }
                        }
                    }
                }
                continue;
            }

            int center_x = ((int)layer->width + dx);
            int center_y = ((int)layer->height + dy);

            int new_fx = center_x - x + (int)layer->dirty_pos_x - 1;
            int new_fy = center_y - y + (int)layer->dirty_pos_y - 1;
            if (new_fx < 0 || new_fy < 0 || new_fx >= framebufferWidth || new_fy >= framebufferHeight)
                continue;

            int new_index = new_fx + new_fy * framebufferWidth;
            uint32_t newPixel = layer->pixels[(layer->width - 1 - x) + (layer->height - 1 - y) * layer->width];

            // Pixel has moved: only update if we were the top layer
            if (GRAPHICS_CONTEXT->top_buffer[index] <= layerIndex)
            {
                // Search downward for next visible pixel
                for (int i = layerIndex - 1; i >= 0; --i)
                {
                    Layer* below = GRAPHICS_LAYERS[i];
                    int local_x = old_fx - below->dirty_pos_x;
                    int local_y = old_fy - below->dirty_pos_y;
                    if (local_x >= 0 && local_x < below->width && local_y >= 0 && local_y < below->height)
                    {
                        uint32_t px = below->pixels[local_x + local_y * below->width];
                        if (!GRAPHICS_IsTransparent(px))
                        {
                            GRAPHICS_CONTEXT->top_buffer[index] = i;
                            // GRAPHICS_CONTEXT->back_buffer[index] = 0xFFFFFFFF;
                            GRAPHICS_CONTEXT->back_buffer[index] = px;
                            break;
                        }
                    }
                }
            }

            if (!GRAPHICS_IsTransparent(newPixel) && GRAPHICS_CONTEXT->top_buffer[new_index] <= layerIndex)
            {
                GRAPHICS_CONTEXT->top_buffer[new_index] = layerIndex;
                // if (GRAPHICS_CONTEXT->back_buffer[new_index] == 0xFFFFFFFF)
                // GRAPHICS_CONTEXT->back_buffer[new_index] = 0xFFAAAAAA; else
                // GRAPHICS_CONTEXT->back_buffer[new_index] = 0xFF000000;
                GRAPHICS_CONTEXT->back_buffer[new_index] = newPixel;
            }
        }
    }

    // Finally update the position
    layer->dirty_pos_x = layer->pos_x;
    layer->dirty_pos_y = layer->pos_y;
}

void GRAPHICS_DrawChar(Layer* layer, uint16_t ch, float* x, float y, int color)
{
    if (ch < FIRST_CHAR || ch >= FIRST_CHAR + NUM_CHARS)
        return;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad((stbtt_bakedchar*)INTEGRATED_FONT->cdata, ATLAS_W, ATLAS_H, ch - FIRST_CHAR, x, &y, &q, 1);

    for (int py = (int)q.y0; py < (int)q.y1; ++py)
    {
        for (int px = (int)q.x0; px < (int)q.x1; ++px)
        {
            int tx = (int)(q.s0 * ATLAS_W + (px - q.x0));
            int ty = (int)(q.t0 * ATLAS_H + (py - q.y0));
            if (px >= 0 && px < layer->width && py >= 0 && py < layer->height && tx >= 0 && tx < ATLAS_W && ty >= 0 && ty < ATLAS_H)
            {
                uint8_t value = INTEGRATED_FONT->atlas[ty][tx];
                color = (color & 0x00FFFFFF) | (value << 24);
                layer->pixels[py * layer->width + px] = BLEND_PIXELS(layer->pixels[py * layer->width + px], color);
            }
        }
    }
}

void GRAPHICS_DrawCharNoInc(Layer* layer, uint16_t ch, float no_inc_x, float y, int color)
{
    if (ch < FIRST_CHAR || ch >= FIRST_CHAR + NUM_CHARS)
        return;

    stbtt_aligned_quad q;
    float x = no_inc_x;
    stbtt_GetBakedQuad(INTEGRATED_FONT->cdata, ATLAS_W, ATLAS_H, ch - FIRST_CHAR, &x, &y, &q, 1);

    for (int py = (int)q.y0; py < (int)q.y1; ++py)
    {
        for (int px = (int)q.x0; px < (int)q.x1; ++px)
        {
            int tx = (int)(q.s0 * ATLAS_W + (px - q.x0));
            int ty = (int)(q.t0 * ATLAS_H + (py - q.y0));
            if (px >= 0 && px < layer->width && py >= 0 && py < layer->height && tx >= 0 && tx < ATLAS_W && ty >= 0 && ty < ATLAS_H)
            {
                uint8_t value = INTEGRATED_FONT->atlas[ty][tx];
                color = (color & 0x00FFFFFF) | (value << 24);
                // if (value > 128)
                layer->pixels[py * layer->width + px] = BLEND_PIXELS(layer->pixels[py * layer->width + px], color);
            }
        }
    }
}

void GRAPHICS_DrawText(Layer* layer, float x, float y, uint8_t* text, KERNEL_color color)
{

    float text_x = x;
    for (int i = 0; text[i]; i++)
    {
        switch (text[i])
        {
        case '\n':
            text_x = x;
            y += INTEGRATED_FONT->font_size;
            break;
        case '\t':
            text_x += 2 * INTEGRATED_FONT->font_size;
            break;
        default:
            GRAPHICS_DrawChar(layer, text[i], &text_x, y, color);
        }
    }
}
