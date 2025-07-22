/* ctype.c - Character type implementation for freestanding environment */

/* For freestanding, we need to define our own types and macros */
#ifndef _CTYPE_H
#define _CTYPE_H

typedef int locale_t;
typedef int __int32_t;

#define _ISbit(bit) (1 << (bit))

enum
{
    _ISupper = _ISbit(0),  /* UPPERCASE. */
    _ISlower = _ISbit(1),  /* lowercase. */
    _ISalpha = _ISbit(2),  /* Alphabetic. */
    _ISdigit = _ISbit(3),  /* Numeric. */
    _ISxdigit = _ISbit(4), /* Hexadecimal numeric. */
    _ISspace = _ISbit(5),  /* Whitespace. */
    _ISprint = _ISbit(6),  /* Printing. */
    _ISgraph = _ISbit(7),  /* Graphical. */
    _ISblank = _ISbit(8),  /* Blank (usually SPC and TAB). */
    _IScntrl = _ISbit(9),  /* Control character. */
    _ISpunct = _ISbit(10), /* Punctuation. */
    _ISalnum = _ISbit(11)  /* Alphanumeric. */
};

#endif /* _CTYPE_H */

/* Simple ASCII-only implementation for freestanding environment */
static const unsigned short int __ctype_b[256] = {
    /* 0-31: control characters */
    [0 ... 31] = _IScntrl,
    /* 32: space */
    [32] = _ISspace | _ISblank | _ISprint,
    /* 33-47: punctuation */
    [33 ... 47] = _ISpunct | _ISgraph | _ISprint,
    /* 48-57: digits */
    [48 ... 57] = _ISdigit | _ISxdigit | _ISgraph | _ISprint | _ISalnum,
    /* 58-64: punctuation */
    [58 ... 64] = _ISpunct | _ISgraph | _ISprint,
    /* 65-70: A-F */
    [65 ... 70] = _ISupper | _ISalpha | _ISxdigit | _ISgraph | _ISprint | _ISalnum,
    /* 71-90: G-Z */
    [71 ... 90] = _ISupper | _ISalpha | _ISgraph | _ISprint | _ISalnum,
    /* 91-96: punctuation */
    [91 ... 96] = _ISpunct | _ISgraph | _ISprint,
    /* 97-102: a-f */
    [97 ... 102] = _ISlower | _ISalpha | _ISxdigit | _ISgraph | _ISprint | _ISalnum,
    /* 103-122: g-z */
    [103 ... 122] = _ISlower | _ISalpha | _ISgraph | _ISprint | _ISalnum,
    /* 123-126: punctuation */
    [123 ... 126] = _ISpunct | _ISgraph | _ISprint,
    /* 127: DEL */
    [127] = _IScntrl,
    /* 128-255: extended ASCII (not handled in basic implementation) */
    [128 ... 255] = 0};

static const __int32_t __ctype_tolower[256] = {['A'] = 'a', ['B'] = 'b', ['C'] = 'c', ['D'] = 'd', ['E'] = 'e', ['F'] = 'f', ['G'] = 'g', ['H'] = 'h', ['I'] = 'i', ['J'] = 'j', ['K'] = 'k', ['L'] = 'l', ['M'] = 'm', ['N'] = 'n', ['O'] = 'o', ['P'] = 'p', ['Q'] = 'q', ['R'] = 'r', ['S'] = 's', ['T'] = 't', ['U'] = 'u', ['V'] = 'v', ['W'] = 'w', ['X'] = 'x', ['Y'] = 'y', ['Z'] = 'z'};

static const __int32_t __ctype_toupper[256] = {['a'] = 'A', ['b'] = 'B', ['c'] = 'C', ['d'] = 'D', ['e'] = 'E', ['f'] = 'F', ['g'] = 'G', ['h'] = 'H', ['i'] = 'I', ['j'] = 'J', ['k'] = 'K', ['l'] = 'L', ['m'] = 'M', ['n'] = 'N', ['o'] = 'O', ['p'] = 'P', ['q'] = 'Q', ['r'] = 'R', ['s'] = 'S', ['t'] = 'T', ['u'] = 'U', ['v'] = 'V', ['w'] = 'W', ['x'] = 'X', ['y'] = 'Y', ['z'] = 'Z'};

/* Accessor functions */
const unsigned short int** __ctype_b_loc(void)
{
    static const unsigned short int* table = __ctype_b;
    return &table;
}

const __int32_t** __ctype_tolower_loc(void)
{
    static const __int32_t* table = __ctype_tolower;
    return &table;
}

const __int32_t** __ctype_toupper_loc(void)
{
    static const __int32_t* table = __ctype_toupper;
    return &table;
}

/* Basic ctype functions */
int isalnum(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISalnum;
}

int isalpha(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISalpha;
}

int iscntrl(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _IScntrl;
}

int isdigit(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISdigit;
}

int islower(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISlower;
}

int isgraph(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISgraph;
}

int isprint(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISprint;
}

int ispunct(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISpunct;
}

int isspace(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISspace;
}

int isupper(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISupper;
}

int isxdigit(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISxdigit;
}

int isblank(int c)
{
    if (c < 0 || c > 255)
        return 0;
    return __ctype_b[c] & _ISblank;
}

/* Case conversion functions */
int tolower(int c)
{
    if (c < 0 || c > 255)
        return c;
    return __ctype_tolower[c] ? __ctype_tolower[c] : c;
}

int toupper(int c)
{
    if (c < 0 || c > 255)
        return c;
    return __ctype_toupper[c] ? __ctype_toupper[c] : c;
}
