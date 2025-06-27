/* MOUSE_STATE->c */
#include <arch/io.h>
#include <arch/pic.h>
#include <drivers/mouse.h>
#include <memory/kglobals.h>

/* Private helper functions */
static void mouse_write(uint8_t data)
{
    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ; /* Wait to write */
    outb(MOUSE_CMD_PORT, 0xD4);
    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ;
    outb(MOUSE_DATA_PORT, data);
}

static uint8_t mouse_read(void)
{
    while (!(inb(MOUSE_STATUS_PORT) & 0x01))
        ; /* Wait for data */
    return inb(MOUSE_DATA_PORT);
}

void mouse_init(void)
{
    /* Enable auxiliary mouse device */
    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ;
    outb(MOUSE_CMD_PORT, 0xA8);

    /* Enable interrupts */
    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ;
    outb(MOUSE_CMD_PORT, 0x20);
    uint8_t status = (inb(MOUSE_DATA_PORT) | 0x02);

    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ;
    outb(MOUSE_CMD_PORT, 0x60);
    while (inb(MOUSE_STATUS_PORT) & 0x02)
        ;
    outb(MOUSE_DATA_PORT, status);

    /* Set default settings */
    mouse_write(0xF6); /* Set defaults */
    mouse_read();
    mouse_write(0xF4); /* Enable mouse */
    mouse_read();

    MOUSE_STATE->enabled = true;

    istream_t istream;
    ostream_t ostream;
    // filesystem_createDevFile(istream, ostream, "mouse");
}

static void handle_mouse_packet(void)
{
    /* Parse buttons */
    MOUSE_STATE->buttons = MOUSE_STATE->packet[0] & 0x07;

    /* Calculate movement with proper sign extension */
    MOUSE_STATE->rel_x = (MOUSE_STATE->packet[0] & MOUSE_X_SIGN)
                             ? (MOUSE_STATE->packet[1] | 0xFFFFFF00)
                             : MOUSE_STATE->packet[1];
    MOUSE_STATE->rel_y = (MOUSE_STATE->packet[0] & MOUSE_Y_SIGN)
                             ? (MOUSE_STATE->packet[2] | 0xFFFFFF00)
                             : MOUSE_STATE->packet[2];

    /* Update absolute position */
    MOUSE_STATE->x += MOUSE_STATE->rel_x;
    MOUSE_STATE->y -= MOUSE_STATE->rel_y; /* Invert Y axis */

    /* Clamp to screen boundaries */
    if (MOUSE_STATE->x < 0)
        MOUSE_STATE->x = 0;
    if (MOUSE_STATE->y < 0)
        MOUSE_STATE->y = 0;
    if (MOUSE_STATE->x >= (int32_t)PREBOOT_INFO->screen_width)
        MOUSE_STATE->x = PREBOOT_INFO->screen_width - 1;
    if (MOUSE_STATE->y >= (int32_t)PREBOOT_INFO->screen_height)
        MOUSE_STATE->y = PREBOOT_INFO->screen_height - 1;
}

void mouse_isr(void)
{
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & 0x01)) /* No data available */
    {
        outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
        return;
    }
    if (!(status & 0x20)) /* Not from mouse */
    {
        outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
        return;
    }

    uint8_t data = inb(MOUSE_DATA_PORT);

    /* Handle packet bytes */
    if (MOUSE_STATE->cycle == 0 && !(data & 0x08))
    {
        /* Bad first byte - reset */
        MOUSE_STATE->cycle = 0;
        return;
    }

    MOUSE_STATE->packet[MOUSE_STATE->cycle++] = data;

    /* Complete packet received */
    if ((!MOUSE_STATE->has_wheel && MOUSE_STATE->cycle == 3) ||
        (MOUSE_STATE->has_wheel && MOUSE_STATE->cycle == 4))
    {
        MOUSE_STATE->cycle = 0;
        handle_mouse_packet();
    }

    /* Send EOI */
    outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

/* Public API implementations */
bool mouse_get_button(uint8_t button)
{
    return (MOUSE_STATE->buttons & button);
}

void mouse_get_position(int32_t* x, int32_t* y)
{
    if (x)
        *x = MOUSE_STATE->x;
    if (y)
        *y = MOUSE_STATE->y;
}