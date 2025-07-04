/**
 * @file keyboard.h
 * @brief PS/2 Keyboard Driver Interface
 *
 * Defines keyboard constants, structures, and functions for handling
 * keyboard input at both low-level (scancodes) and high-level (key events).
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <arch/io.h>
#include <kint.h>

/* Key modifier flags (bitmask) */
#define KEY_MOD_CTRL (1 << 0)    ///< Control key modifier
#define KEY_MOD_SHIFT (1 << 1)   ///< Shift key modifier
#define KEY_MOD_ALT (1 << 2)     ///< Alt key modifier
#define KEY_MOD_ALTGR (1 << 3)   ///< AltGr key modifier
#define KEY_MOD_CAPS (1 << 4)    ///< Caps Lock state
#define KEY_MOD_NUMLOCK (1 << 5) ///< Num Lock state
#define KEY_MOD_SCROLL (1 << 6)  ///< Scroll Lock state

/**
 * @enum Special key codes
 * @brief Key codes for non-ASCII keys (values above standard ASCII range)
 */
enum
{
    KEY_NONE = 0,         ///< No key pressed
    KEY_ESCAPE = 0x1B,    ///< Escape key
    KEY_BACKSPACE = 0x08, ///< Backspace key
    KEY_TAB = 0x09,       ///< Tab key
    KEY_ENTER = 0x0D,     ///< Enter/Return key
    KEY_DELETE = 0x7F,    ///< Delete key

    /* Function keys */
    KEY_F1 = 0x80, ///< F1 key
    KEY_F2,        ///< F2 key
    KEY_F3,        ///< F3 key
    KEY_F4,        ///< F4 key
    KEY_F5,        ///< F5 key
    KEY_F6,        ///< F6 key
    KEY_F7,        ///< F7 key
    KEY_F8,        ///< F8 key
    KEY_F9,        ///< F9 key
    KEY_F10,       ///< F10 key
    KEY_F11,       ///< F11 key
    KEY_F12,       ///< F12 key

    /* Navigation keys */
    KEY_HOME,      ///< Home key
    KEY_END,       ///< End key
    KEY_PAGE_UP,   ///< Page Up key
    KEY_PAGE_DOWN, ///< Page Down key
    KEY_UP,        ///< Up arrow key
    KEY_DOWN,      ///< Down arrow key
    KEY_LEFT,      ///< Left arrow key
    KEY_RIGHT,     ///< Right arrow key
    KEY_INSERT,    ///< Insert key
    KEY_POWER,     ///< Power key
    KEY_SLEEP,     ///< Sleep key
    KEY_WAKE,      ///< Wake key

    /* Special keys */
    KEY_PRINT_SCREEN, ///< Print Screen key
    KEY_PAUSE,        ///< Pause/Break key
    KEY_MENU,         ///< Context Menu key
    KEY_LWIN,         ///< Left Windows key
    KEY_RWIN,         ///< Right Windows key

    /* Keypad keys */
    KEY_KP_ENTER,    ///< Keypad Enter
    KEY_KP_PLUS,     ///< Keypad +
    KEY_KP_MINUS,    ///< Keypad -
    KEY_KP_MULTIPLY, ///< Keypad *
    KEY_KP_DIVIDE,   ///< Keypad /
    KEY_KP_DOT,      ///< Keypad .
    KEY_KP_0,        ///< Keypad 0
    KEY_KP_1,        ///< Keypad 1
    KEY_KP_2,        ///< Keypad 2
    KEY_KP_3,        ///< Keypad 3
    KEY_KP_4,        ///< Keypad 4
    KEY_KP_5,        ///< Keypad 5
    KEY_KP_6,        ///< Keypad 6
    KEY_KP_7,        ///< Keypad 7
    KEY_KP_8,        ///< Keypad 8
    KEY_KP_9,        ///< Keypad 9

    KEY_LAST = 0xFF ///< Last key code value
};

/**
 * @struct key_event_t
 * @brief Keyboard event structure containing key press/release information
 */
typedef struct
{
    uint8_t scancode;  ///< Raw hardware scancode
    uint8_t keycode;   ///< Translated keycode (ASCII or special key)
    uint8_t modifiers; ///< Current modifier state (KEY_MOD_* flags)
    bool pressed;      ///< True if key was pressed, false if released
    bool is_extended;  ///< True if key uses extended scancode (0xE0 prefix)
} key_event_t;

/**
 * @struct keyboard_state_t
 * @brief Keyboard driver state structure
 */
typedef struct
{
    key_event_t event_queue[64];   ///< Circular buffer for key events
    uint8_t head;                  ///< Index of next write position
    uint8_t tail;                  ///< Index of next read position
    uint8_t modifiers;             ///< Current modifier state (bitmask)
    bool caps_lock;                ///< Caps Lock toggle state
    bool num_lock;                 ///< Num Lock toggle state
    bool scroll_lock;              ///< Scroll Lock toggle state
    bool extended;                 ///< Next key is extended (0xE0 seen)
    struct file_descriptor_t* dev; ///< Associated device file descriptor
} keyboard_state_t;

/* ========================== Public API functions ================================= */

/**
 * @brief Initialize the PS/2 keyboard driver
 *
 * Must be called before any other keyboard functions.
 */
void keyboard_init(void);

/**
 * @brief Check for pending keyboard events
 *
 * @return true if there are unread key events in the queue
 * @return false if no events are available
 *
 * This function can be used to check if keyboard_get_event()
 * will return data without blocking.
 */
bool keyboard_has_input(void);

/**
 * @brief Get the keyboard device file descriptor
 *
 * @return Pointer to keyboard device file descriptor structure
 *
 * The returned file descriptor can be used with standard file operations
 * to read keyboard events. Each read returns a key_event_t structure.
 */
struct file_descriptor_t* keyboard_get_dev();

/**
 * @brief Keyboard interrupt service routine
 *
 * This function should only be called by the interrupt handler.
 * Automatically sends EOI to PIC when complete.
 */
void keyboard_isr(void);

#endif /* KEYBOARD_H */