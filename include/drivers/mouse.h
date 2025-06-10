#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>
#include <stdint.h>

#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_CMD_PORT 0x64

/* Mouse packet bits */
#define MOUSE_LEFT_BTN (1 << 0)
#define MOUSE_RIGHT_BTN (1 << 1)
#define MOUSE_MIDDLE_BTN (1 << 2)
#define MOUSE_X_SIGN (1 << 4)
#define MOUSE_Y_SIGN (1 << 5)
#define MOUSE_X_OVERFLOW (1 << 6)
#define MOUSE_Y_OVERFLOW (1 << 7)

typedef struct
{
    int32_t x;         /* Absolute X position */
    int32_t y;         /* Absolute Y position */
    int32_t rel_x;     /* Relative X movement */
    int32_t rel_y;     /* Relative Y movement */
    uint8_t buttons;   /* Bitmask of pressed buttons */
    uint8_t packet[4]; /* Packet buffer (supports scroll wheel) */
    uint8_t cycle;     /* Current packet byte */
    bool enabled;      /* Mouse activation status */
    bool has_wheel;    /* Wheel detection flag */

} mouse_t;

/* Public API */
void mouse_init(void);
void mouse_isr(void);
bool mouse_get_button(uint8_t button);
void mouse_get_position(int32_t* x, int32_t* y);

#endif