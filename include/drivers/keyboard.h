/* keyboard.h */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <arch/io.h>
#include <fs/stream.h>
#include <kint.h>

/* Key modifier flags */
#define KEY_MOD_CTRL (1 << 0)
#define KEY_MOD_SHIFT (1 << 1)
#define KEY_MOD_ALT (1 << 2)
#define KEY_MOD_ALTGR (1 << 3)
#define KEY_MOD_CAPS (1 << 4)
#define KEY_MOD_NUMLOCK (1 << 5)
#define KEY_MOD_SCROLL (1 << 6)

/* Special key codes (above ASCII range) */
enum
{
    KEY_NONE = 0,
    KEY_ESCAPE = 0x1B,
    KEY_BACKSPACE = 0x08,
    KEY_TAB = 0x09,
    KEY_ENTER = 0x0D,
    KEY_DELETE = 0x7F,

    /* Function keys */
    KEY_F1 = 0x80,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,

    /* Navigation keys */
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_INSERT,
    KEY_POWER,
    KEY_SLEEP,
    KEY_WAKE,

    /* Special keys */
    KEY_PRINT_SCREEN,
    KEY_PAUSE,
    KEY_MENU,
    KEY_LWIN,
    KEY_RWIN,

    /* Keypad keys */
    KEY_KP_ENTER,
    KEY_KP_PLUS,
    KEY_KP_MINUS,
    KEY_KP_MULTIPLY,
    KEY_KP_DIVIDE,
    KEY_KP_DOT,
    KEY_KP_0,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,

    KEY_LAST = 0xFF
};

typedef struct
{
    uint8_t scancode;  /* Raw scancode */
    uint8_t keycode;   /* Translated keycode */
    uint8_t modifiers; /* Modifier state */
    bool pressed;      /* Press (true) or release (false) */
    bool is_extended;  /* Extended key (0xE0 prefix) */
} key_event_t;

/* Keyboard state structure */
typedef struct
{
    key_event_t event_queue[64]; /* Circular event buffer */
    uint8_t head;                /* Buffer head index */
    uint8_t tail;                /* Buffer tail index */
    uint8_t modifiers;           /* Current modifier state */
    bool caps_lock;              /* Caps Lock state */
    bool num_lock;               /* Num Lock state */
    bool scroll_lock;            /* Scroll Lock state */
    bool extended;               /* Next key is extended (0xE0 seen) */
    struct open_file_t* dev;     /* keyboard device */
} keyboard_state_t;

/* Public API */
void keyboard_init(void);
bool keyboard_has_input(void);
struct open_file_t* keyboard_get_dev();
void keyboard_isr(void);

#endif KEYBOARD_H