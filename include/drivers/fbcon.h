/* fbcon.h */
#ifndef FB_CON_H
#define FB_CON_H

#include <drivers/graphics.h>
#include <drivers/tty.h>
#include <fs/fontLoader.h>
#include <kint.h>

typedef struct
{
    size_t stream_ptr;
    tty_t* tty;          // Linked TTY instance
    Font* font;          // Active font
    Layer* layer;        // Drawing surface
    uint32_t bg_color;   // Background color
    uint32_t text_color; // Default text color
    uint32_t cursor_x;   // Cursor position
    uint32_t cursor_y;
    bool cursor_visible; // Cursor state
    uint64_t last_blink; // For cursor blinking
    bool enabled;
} fbcon_t;

void fbcon_init(tty_t* tty, Font* fon);
void fbcon_update();
void fbcon_enable();
void fbcon_disable();
fbcon_t* fbcon_get();

#endif