/* integratedTerminal.c - Enhanced fbcon implementation */
#include <drivers/fbcon.h>
#include <drivers/graphics.h>
#include <drivers/keyboard.h>
#include <fs/fontLoader.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/paging.h>
#include <misc/debug.h>

static void draw_cursor(bool visible)
{
    if (!FBCON->cursor_visible && !visible)
        return;

    // Simple block cursor
    uint32_t color = visible ? FBCON->text_color : FBCON->bg_color;
    GRAPHICS_FillRect(FBCON->layer, FBCON->cursor_x, FBCON->cursor_y - (FBCON->font->height * 2),
                      FBCON->font->width, FBCON->font->height, color);
}

static void render_row(tty_row_t* row, int* y_pos, int* x_pos)
{
    uint32_t color = FBCON->text_color;

    // Special row types can have different colors
    if (row->type == TTY_ROW_COMMAND)
    {
        color = 0xFF00FF00; // Green for commands
    }
    else if (row->type == TTY_ROW_EXTRA)
    {
        color = 0xFF8888FF; // Light blue for info
    }

    // Clear row background
    GRAPHICS_FillRect(FBCON->layer, 0, *y_pos, FBCON->layer->width, FBCON->font->height,
                      FBCON->bg_color);

    // Draw text
    *x_pos = 0;
    for (uint32_t i = 0; i < row->length; i++)
    {
        char ch = row->chars[i];

        if (i == TTY_ROW_MAX_CHARS - 10)
        {
            GRAPHICS_DrawCharNoInc(FBCON->layer, '.', *x_pos, *y_pos, color);
            *x_pos += FBCON->font->width;
            GRAPHICS_DrawCharNoInc(FBCON->layer, '.', *x_pos, *y_pos, color);
            *x_pos += FBCON->font->width;
            GRAPHICS_DrawCharNoInc(FBCON->layer, '.', *x_pos, *y_pos, color);
            return;
        }

        switch (ch)
        {
        case '\t':
            *x_pos += FBCON->font->width * 4; // Tab = 4 spaces
            break;
        default:
            GRAPHICS_DrawCharNoInc(FBCON->layer, ch, *x_pos, *y_pos, color);
            *x_pos += FBCON->font->width;
        }
        if (i % 150 == 149)
        {
            *x_pos = 0;
            *y_pos += FBCON->font->height;
        }
    }
}

void fbcon_render()
{
    if (!FBCON->tty || !FBCON->tty->head)
        return;

    // Clear entire layer periodically (or use dirty rectangle tracking)
    GRAPHICS_FillRect(FBCON->layer, 0, 0, FBCON->layer->width, FBCON->layer->height,
                      FBCON->bg_color);

    // Render all rows
    int y_pos = FBCON->font->height;
    int x_pos = 0;
    tty_row_t* current = FBCON->tty->head;
    while (current && y_pos < FBCON->layer->height)
    {
        render_row(current, &y_pos, &x_pos);
        y_pos += FBCON->font->height;
        current = current->next;
    }

    // Draw curor on current row
    if (FBCON->tty->current)
    {
        FBCON->cursor_x = x_pos; // FBCON->tty->current->length * FBCON->font->width;
        FBCON->cursor_y = y_pos; //(FBCON->tty->row_count - 1) * FBCON->font->height;
        draw_cursor(true);
    }
    FBCON->layer->pos_x = 0;
    FBCON->layer->pos_y = 0;
    GRAPHICS_UpdateLayer(FBCON->layer);
}

void fbcon_init(tty_t* tty, Font* font)
{
    memset(FBCON, 0, sizeof(fbcon_t));
    fbcon_enable();

    FBCON->tty = tty;
    FBCON->font = INTEGRATED_FONT;
    FBCON->layer = GRAPHICS_CreateLayer(kmalloc(sizeof(Layer)),
                                        kmalloc(sizeof(uint32_t) * 1920 * 1080), 1920, 1080);
    FBCON->bg_color = 0xFF000000;   // Black background
    FBCON->text_color = 0xFFFFFFFF; // White text
    FBCON->cursor_visible = true;
    FBCON->stream_ptr = 0;

    GRAPHICS_FillRect(FBCON->layer, 0, 0, 1920, 1080, FBCON->bg_color);
    GRAPHICS_ApplyLayer(FBCON->layer);

    tty->echo = true;
    fbcon_render(FBCON);
}

void fbcon_enable()
{
    FBCON->enabled = true;
}

void fbcon_disable()
{
    FBCON->enabled = false;
}

void fbcon_update()
{
    if (!FBCON->enabled)
        return;

    draw_cursor(FBCON->cursor_visible);

    dev_file_t* keyboard = keyboard_get_dev();
    while (keyboard_has_input())
    {
        key_event_t key;
        if (dev_kernel_fn(KEYBOARD_STATE->dev->dev_id, DEV_READ, (uint64_t)(&key),
                          sizeof(key_event_t)) == sizeof(key_event_t) &&
            key.pressed)
        {

            tty_handle_key(FBCON->tty, key.keycode, key.modifiers);
        }
    }

    fbcon_render(FBCON);
}