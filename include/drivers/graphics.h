/**
 * @file graphics.h
 * @brief Kernel Graphics Subsystem Interface
 *
 * Declares types, constants, and function prototypes for framebuffer graphics and drawing routines.
 */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <kint.h>

typedef uint32_t KERNEL_color;

#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_RED 0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE 0xFF0000FF
#define COLOR_YELLOW 0xFFFFFF00
#define COLOR_MAGENTA 0xFFFF00FF
#define COLOR_CYAN 0xFF00FFFF
#define COLOR_TRANSPARENT 0x00000000

#define ALPHA_ARGB(color) (((color) >> 24) & 0xFF)
#define RED_ARGB(color) (((color) >> 16) & 0xFF)
#define GREEN_ARGB(color) (((color) >> 8) & 0xFF)
#define BLUE_ARGB(color) ((color)&0xFF)

#define BLEND_PIXELS(dst, src)                                                                     \
    (((uint32_t)(((((src >> 24) & 0xFF) * ((src >> 24) & 0xFF)) >> 8) +                            \
                 (((dst >> 24) & 0xFF) * (256 - ((src >> 24) & 0xFF)) >> 8))                       \
      << 24) |                                                                                     \
     ((uint32_t)(((((src >> 16) & 0xFF) * ((src >> 24) & 0xFF)) >> 8) +                            \
                 (((dst >> 16) & 0xFF) * (256 - ((src >> 24) & 0xFF)) >> 8))                       \
      << 16) |                                                                                     \
     ((uint32_t)(((((src >> 8) & 0xFF) * ((src >> 24) & 0xFF)) >> 8) +                             \
                 (((dst >> 8) & 0xFF) * (256 - ((src >> 24) & 0xFF)) >> 8))                        \
      << 8) |                                                                                      \
     ((uint32_t)(((((src >> 0) & 0xFF) * ((src >> 24) & 0xFF)) >> 8) +                             \
                 (((dst >> 0) & 0xFF) * (256 - ((src >> 24) & 0xFF)) >> 8))                        \
      << 0))

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} image;

typedef struct
{
    uint32_t* pixels;
    uint32_t width;
    uint32_t height;
    float scale_x;
    float scale_y;
    uint32_t pos_x;
    uint32_t pos_y;

    float dirty_scale_x;
    float dirty_scale_y;
    uint32_t dirty_pos_x;
    uint32_t dirty_pos_y;
    bool visible;
    uint32_t layer;
} Layer;

typedef struct GraphicsContext
{
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t back_buffer_size;
    uint32_t* back_buffer;
    uint32_t* framebuffer;
    uint32_t* top_buffer;
} GraphicsContext;

void GRAPHICS_InitGraphics(void* top);

void GRAPHICS_CleanupGraphics();

void GRAPHICS_Clear(KERNEL_color color);

void GRAPHICS_DrawPixel(Layer* layer, int32_t x, int32_t y, int32_t color);

void GRAPHICS_SafeDrawPixel(Layer* layer, int32_t x, int32_t y, KERNEL_color color);

void GRAPHICS_DrawLine(Layer* layer,
                       int32_t x1,
                       int32_t y1,
                       int32_t x2,
                       int32_t y2,
                       KERNEL_color color);

void GRAPHICS_DrawRect(Layer* layer,
                       int32_t x,
                       int32_t y,
                       int32_t w,
                       int32_t h,
                       KERNEL_color color);

void GRAPHICS_FillRect(Layer* layer,
                       int32_t x,
                       int32_t y,
                       int32_t w,
                       int32_t h,
                       KERNEL_color color);

void GRAPHICS_DrawCircle(Layer* layer,
                         int32_t cx,
                         int32_t cy,
                         uint32_t r,
                         uint32_t line_thickness,
                         KERNEL_color color);

void GRAPHICS_FillCircle(Layer* layer, int32_t cx, int32_t cy, uint32_t r, KERNEL_color color);

Layer* GRAPHICS_CreateLayer(void* layer, void* pixels, uint32_t w, uint32_t h);

void GRAPHICS_DestroyLayer(Layer* layer);

void GRAPHICS_ClearLayer(Layer* layer, KERNEL_color color);

void GRAPHICS_ApplyLayerOverride(Layer* layer, uint32_t index);

void GRAPHICS_ApplyLayer(Layer* layer);

uint32_t GRAPHICS_GetLayerPixel(Layer* layer, int x, int y);

bool GRAPHICS_IsTransparent(KERNEL_color color);

void GRAPHICS_RemoveLayer(Layer* layer);

void GRAPHICS_UpdateLayer(Layer* layer);

void GRAPHICS_DrawChar(Layer* layer, uint16_t ch, float* x, float y, int color);

void GRAPHICS_DrawCharNoInc(Layer* layer, uint16_t ch, float no_inc_x, float y, int color);

void GRAPHICS_DrawText(Layer* layer, float x, float y, uint8_t* text, KERNEL_color color);

#endif