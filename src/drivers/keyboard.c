/* keyboard.c - Complete PS/2 Keyboard Driver */
#include <arch/pic.h>
#include <drivers/keyboard.h>

#include <fs/fdm.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <misc/debug.h>

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_CMD_PORT 0x64

/* Normal (unshifted) keys - scancode set 1 */
static const uint8_t scancode_normal[128] = {
    [0x00] = KEY_NONE,        /* Invalid */
    [0x01] = KEY_ESCAPE,      /* Escape */
    [0x02] = '1',             /* 1 */
    [0x03] = '2',             /* 2 */
    [0x04] = '3',             /* 3 */
    [0x05] = '4',             /* 4 */
    [0x06] = '5',             /* 5 */
    [0x07] = '6',             /* 6 */
    [0x08] = '7',             /* 7 */
    [0x09] = '8',             /* 8 */
    [0x0A] = '9',             /* 9 */
    [0x0B] = '0',             /* 0 */
    [0x0C] = '-',             /* - */
    [0x0D] = '=',             /* = */
    [0x0E] = '\b',            /* Backspace */
    [0x0F] = KEY_TAB,         /* Tab */
    [0x10] = 'q',             /* Q */
    [0x11] = 'w',             /* W */
    [0x12] = 'e',             /* E */
    [0x13] = 'r',             /* R */
    [0x14] = 't',             /* T */
    [0x15] = 'y',             /* Y */
    [0x16] = 'u',             /* U */
    [0x17] = 'i',             /* I */
    [0x18] = 'o',             /* O */
    [0x19] = 'p',             /* P */
    [0x1A] = '[',             /* [ */
    [0x1B] = ']',             /* ] */
    [0x1C] = '\n',            /* Enter */
    [0x1D] = KEY_MOD_CTRL,    /* Left Ctrl */
    [0x1E] = 'a',             /* A */
    [0x1F] = 's',             /* S */
    [0x20] = 'd',             /* D */
    [0x21] = 'f',             /* F */
    [0x22] = 'g',             /* G */
    [0x23] = 'h',             /* H */
    [0x24] = 'j',             /* J */
    [0x25] = 'k',             /* K */
    [0x26] = 'l',             /* L */
    [0x27] = ';',             /* ; */
    [0x28] = '\'',            /* ' */
    [0x29] = '`',             /* ` */
    [0x2A] = KEY_MOD_SHIFT,   /* Left Shift */
    [0x2B] = '\\',            /* \ */
    [0x2C] = 'z',             /* Z */
    [0x2D] = 'x',             /* X */
    [0x2E] = 'c',             /* C */
    [0x2F] = 'v',             /* V */
    [0x30] = 'b',             /* B */
    [0x31] = 'n',             /* N */
    [0x32] = 'm',             /* M */
    [0x33] = ',',             /* , */
    [0x34] = '.',             /* . */
    [0x35] = '/',             /* / */
    [0x36] = KEY_MOD_SHIFT,   /* Right Shift */
    [0x37] = '*',             /* Keypad * */
    [0x38] = KEY_MOD_ALT,     /* Left Alt */
    [0x39] = ' ',             /* Space */
    [0x3A] = KEY_MOD_CAPS,    /* Caps Lock */
    [0x3B] = KEY_F1,          /* F1 */
    [0x3C] = KEY_F2,          /* F2 */
    [0x3D] = KEY_F3,          /* F3 */
    [0x3E] = KEY_F4,          /* F4 */
    [0x3F] = KEY_F5,          /* F5 */
    [0x40] = KEY_F6,          /* F6 */
    [0x41] = KEY_F7,          /* F7 */
    [0x42] = KEY_F8,          /* F8 */
    [0x43] = KEY_F9,          /* F9 */
    [0x44] = KEY_F10,         /* F10 */
    [0x45] = KEY_MOD_NUMLOCK, /* Num Lock */
    [0x46] = KEY_MOD_SCROLL,  /* Scroll Lock */
    [0x47] = KEY_HOME,        /* Keypad 7/Home */
    [0x48] = KEY_UP,          /* Keypad 8/Up */
    [0x49] = KEY_PAGE_UP,     /* Keypad 9/PgUp */
    [0x4A] = '-',             /* Keypad - */
    [0x4B] = KEY_LEFT,        /* Keypad 4/Left */
    [0x4C] = '5',             /* Keypad 5 */
    [0x4D] = KEY_RIGHT,       /* Keypad 6/Right */
    [0x4E] = '+',             /* Keypad + */
    [0x4F] = '\n',            /* Keypad 1/End */
    [0x50] = KEY_DOWN,        /* Keypad 2/Down */
    [0x51] = KEY_PAGE_DOWN,   /* Keypad 3/PgDn */
    [0x52] = KEY_INSERT,      /* Keypad 0/Ins */
    [0x53] = KEY_DELETE,      /* Keypad ./Del */
    /* 0x54-0x56 - International keys */
    [0x57] = KEY_F11, /* F11 */
    [0x58] = KEY_F12, /* F12 */
    /* 0x59-0x7F - Reserved/Extended */
};

/* Shifted keys - scancode set 1 with Shift modifier */
static const uint8_t scancode_shift[128] = {
    [0x02] = '!', /* Shift+1 */
    [0x03] = '@', /* Shift+2 */
    [0x04] = '#', /* Shift+3 */
    [0x05] = '$', /* Shift+4 */
    [0x06] = '%', /* Shift+5 */
    [0x07] = '^', /* Shift+6 */
    [0x08] = '&', /* Shift+7 */
    [0x09] = '*', /* Shift+8 */
    [0x0A] = '(', /* Shift+9 */
    [0x0B] = ')', /* Shift+0 */
    [0x0C] = '_', /* Shift+- */
    [0x0D] = '+', /* Shift+= */
    [0x10] = 'Q', /* Shift+Q */
    [0x11] = 'W', /* Shift+W */
    [0x12] = 'E', /* Shift+E */
    [0x13] = 'R', /* Shift+R */
    [0x14] = 'T', /* Shift+T */
    [0x15] = 'Y', /* Shift+Y */
    [0x16] = 'U', /* Shift+U */
    [0x17] = 'I', /* Shift+I */
    [0x18] = 'O', /* Shift+O */
    [0x19] = 'P', /* Shift+P */
    [0x1A] = '{', /* Shift+[ */
    [0x1B] = '}', /* Shift+] */
    [0x1E] = 'A', /* Shift+A */
    [0x1F] = 'S', /* Shift+S */
    [0x20] = 'D', /* Shift+D */
    [0x21] = 'F', /* Shift+F */
    [0x22] = 'G', /* Shift+G */
    [0x23] = 'H', /* Shift+H */
    [0x24] = 'J', /* Shift+J */
    [0x25] = 'K', /* Shift+K */
    [0x26] = 'L', /* Shift+L */
    [0x27] = ':', /* Shift+; */
    [0x28] = '"', /* Shift+' */
    [0x29] = '~', /* Shift+` */
    [0x2B] = '|', /* Shift+\ */
    [0x2C] = 'Z', /* Shift+Z */
    [0x2D] = 'X', /* Shift+X */
    [0x2E] = 'C', /* Shift+C */
    [0x2F] = 'V', /* Shift+V */
    [0x30] = 'B', /* Shift+B */
    [0x31] = 'N', /* Shift+N */
    [0x32] = 'M', /* Shift+M */
    [0x33] = '<', /* Shift+, */
    [0x34] = '>', /* Shift+. */
    [0x35] = '?'  /* Shift+/ */
};

/* Extended keys (0xE0 prefix) */
static const uint8_t scancode_extended[128] = {
    [0x1D] = KEY_MOD_CTRL,  /* Right Ctrl */
    [0x38] = KEY_MOD_ALTGR, /* Right Alt */
    [0x47] = KEY_HOME,      /* Home */
    [0x48] = KEY_UP,        /* Up */
    [0x49] = KEY_PAGE_UP,   /* Page Up */
    [0x4B] = KEY_LEFT,      /* Left */
    [0x4D] = KEY_RIGHT,     /* Right */
    [0x4F] = KEY_END,       /* End */
    [0x50] = KEY_DOWN,      /* Down */
    [0x51] = KEY_PAGE_DOWN, /* Page Down */
    [0x52] = KEY_INSERT,    /* Insert */
    [0x53] = KEY_DELETE,    /* Delete */
    [0x5B] = KEY_LWIN,      /* Left Win */
    [0x5C] = KEY_RWIN,      /* Right Win */
    [0x5D] = KEY_MENU,      /* Menu */
    [0x5E] = KEY_POWER,     /* Power */
    [0x5F] = KEY_SLEEP,     /* Sleep */
    [0x63] = KEY_WAKE       /* Wake */
};

/**
 * @brief Read a key event from the keyboard queue
 * @param open_file Device file descriptor
 * @param event_dest Buffer to store the key event
 * @param _size Size of destination buffer
 * @return Number of bytes read (sizeof(key_event_t)) or 0 if queue empty
 */
size_t keyboard_get_event(file_descriptor_t* open_file, key_event_t* event_dest, size_t _size)
{
    /* Original code remains exactly the same */
    if (!keyboard_has_input())
    {
        return 0;
    }

    key_event_t event = KEYBOARD_STATE->event_queue[KEYBOARD_STATE->tail];
    KEYBOARD_STATE->tail = (KEYBOARD_STATE->tail + 1) % (sizeof(KEYBOARD_STATE->event_queue) / sizeof(KEYBOARD_STATE->event_queue[0]));
    kmemcpy(event_dest, &event, sizeof(key_event_t));
    return sizeof(key_event_t);
}

/**
 * @brief Write a key event to the keyboard queue
 * @param event_src Event to add to the queue
 *
 * Implements a circular buffer for keyboard events. If the buffer is full,
 * overwrites the oldest event.
 */
static void keyboard_write_event(const key_event_t* event_src)
{
    /* Original code remains exactly the same */
    size_t queue_size = sizeof(KEYBOARD_STATE->event_queue) / sizeof(KEYBOARD_STATE->event_queue[0]);

    KEYBOARD_STATE->event_queue[KEYBOARD_STATE->head] = *event_src;
    KEYBOARD_STATE->head = (KEYBOARD_STATE->head + 1) % queue_size;

    if (KEYBOARD_STATE->head == KEYBOARD_STATE->tail)
    {
        KEYBOARD_STATE->tail = (KEYBOARD_STATE->tail + 1) % queue_size;
    }
}

/**
 * @brief Get the keyboard device file descriptor
 * @return Pointer to keyboard device file descriptor
 */
file_descriptor_t* keyboard_get_dev()
{
    /* Original code remains exactly the same */
    return KEYBOARD_STATE->dev;
}

/**
 * @brief Process a raw keyboard scancode into a key event
 * @param scancode Raw scancode from keyboard
 * @return Fully populated key_event_t structure
 *
 * Handles:
 * - Key press/release states
 * - Modifier keys
 * - Extended keys (0xE0 prefix)
 * - Toggle keys (Caps Lock, Num Lock, Scroll Lock)
 */
key_event_t process_scancode(uint64_t scancode)
{
    bool pressed = !(scancode & 0x80);
    uint8_t code = scancode & 0x7F;

    key_event_t event = {.scancode = scancode, .pressed = pressed, .modifiers = KEYBOARD_STATE->modifiers, .is_extended = KEYBOARD_STATE->extended};

    /* Handle extended key prefix */
    if (scancode == 0xE0)
    {
        KEYBOARD_STATE->extended = true;
    }

    /* Translate scancode to keycode */
    if (KEYBOARD_STATE->extended)
    {
        event.keycode = scancode_extended[code];
        KEYBOARD_STATE->extended = false;
    }
    else if (KEYBOARD_STATE->modifiers & KEY_MOD_SHIFT)
    {
        event.keycode = scancode_shift[code] ? scancode_shift[code] : scancode_normal[code];
    }
    else
    {
        event.keycode = scancode_normal[code];
    }

    /* Update modifier state */
    switch (event.keycode)
    {
    case KEY_MOD_CTRL:
        if (pressed)
            KEYBOARD_STATE->modifiers |= KEY_MOD_CTRL;
        else
            KEYBOARD_STATE->modifiers &= ~KEY_MOD_CTRL;
        break;
    case KEY_MOD_SHIFT:
        if (pressed)
            KEYBOARD_STATE->modifiers |= KEY_MOD_SHIFT;
        else
            KEYBOARD_STATE->modifiers &= ~KEY_MOD_SHIFT;
        break;
    case KEY_MOD_ALT:
        if (pressed)
            KEYBOARD_STATE->modifiers |= KEY_MOD_ALT;
        else
            KEYBOARD_STATE->modifiers &= ~KEY_MOD_ALT;
        break;
    case KEY_MOD_CAPS:
        if (pressed)
            KEYBOARD_STATE->caps_lock = !KEYBOARD_STATE->caps_lock;
        break;
    case KEY_MOD_NUMLOCK:
        if (pressed)
            KEYBOARD_STATE->num_lock = !KEYBOARD_STATE->num_lock;
        break;
    case KEY_MOD_SCROLL:
        if (pressed)
            KEYBOARD_STATE->scroll_lock = !KEYBOARD_STATE->scroll_lock;
        break;
    default:
        break;
    }

    /* Apply Caps Lock to letters */
    if ((KEYBOARD_STATE->caps_lock) && (event.keycode >= 'a' && event.keycode <= 'z'))
    {
        if (pressed)
        {
            event.keycode = event.keycode - 'a' + 'A';
        }
    }

    return event;
}

/**
 * @brief Check if keyboard has pending events
 * @return true if events available, false otherwise
 */
bool keyboard_has_input(void)
{
    /* Original code remains exactly the same */
    return KEYBOARD_STATE->head != KEYBOARD_STATE->tail;
}

/**
 * @brief Keyboard interrupt service routine (IRQ1)
 *
 * Reads scancodes from keyboard controller, processes them into events,
 * and adds them to the event queue.
 */
void keyboard_isr(void)
{
    /* Original code remains exactly the same */
    uint8_t status = inb(KBD_STATUS_PORT);
    if (!(status & 0x01))
    {
        outb(PIC1_CMD, PIC_EOI);
        return;
    }

    uint8_t scancode = inb(KBD_DATA_PORT);
    key_event_t event = process_scancode(scancode);
    keyboard_write_event(&event);
    outb(PIC1_CMD, PIC_EOI);
}

/**
 * @brief Initialize the keyboard driver
 *
 * Sets up:
 * - Keyboard hardware
 * - Event queue
 * - Device file
 * - Interrupt handling
 */
void keyboard_init(void)
{
    /* Original code remains exactly the same */
    memset(KEYBOARD_STATE, 0, sizeof(keyboard_state_t));

    outb(KBD_STATUS_PORT, 0xAE);
    outb(KBD_STATUS_PORT, 0xA8);
    uint8_t ack = inb(KBD_DATA_PORT);
    if (ack != 0xFA)
    {
        /* Handle error */
    }

    vfs_entry_t* device_file = vfs_create_entry(*DEV, "keyboard", EXT2_FT_CHRDEV);
    device_file->ops[DEV_READ] = keyboard_get_event;
    device_file->private_data = KEYBOARD_STATE;
    KEYBOARD_STATE->dev = fdm_open_file(device_file);
}