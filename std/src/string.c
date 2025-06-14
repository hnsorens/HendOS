/* kstring.c */
#include <stdint.h>
#include <string.h>

/**
 * @brief Calculates the length of a null-terminated 8-bit string
 */
size_t strlen(const uint8_t* str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

/**
 * @brief Calcualtes the length of a null-terminated 16-bit string
 */
size_t strlen16(const uint16_t* str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

/**
 * @brief Copy an 8-bit string from source to destination
 */
uint8_t* strcpy(uint8_t* dest, const uint8_t* src)
{
    size_t i = 0;
    while ((dest[i] = src[i]))
    {
        i++;
    }
    return dest;
}

/**
 * @brief Copy an 16-bit string from source to destination
 */
uint16_t* strcpy16(uint16_t* dest, const uint16_t* src)
{
    size_t i = 0;
    while ((dest[i] = src[i]))
    {
        i++;
    }
    return dest;
}

/**
 * @brief Copy up to n characters from an 8-bit string
 */
uint8_t* strncpy(uint8_t* dest, const uint8_t* src, int n)
{
    size_t i = 0;
    while (i < n && src[i])
    {
        dest[i] = src[i];
        i++;
    }
    while (i < n)
    {
        dest[i++] = '\0';
    }
    return dest;
}

/**
 * @brief Copy up to n characters from an 16-bit string
 */
uint16_t* strncpy16(uint16_t* dest, const uint16_t* src, int n)
{
    size_t i = 0;
    while (i < n && src[i])
    {
        dest[i] = src[i];
        i++;
    }
    while (i < n)
    {
        dest[i++] = '\0';
    }
    return dest;
}

/**
 * @brief Compare two 8-bit strings
 */
uint8_t strcmp(const uint8_t* s1, const uint8_t* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/**
 * @brief Compare two 16-bit strings
 */
uint16_t strcmp16(const uint16_t* s1, const uint16_t* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/**
 * @brief Compares a 16-bit string with an 8-bit string
 */
uint16_t strcmp_16_8(const uint16_t* s1, const uint8_t* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/**
 * @brief Compare first n characters of 2 8-bit strings
 */
uint8_t strncmp(const uint8_t* s1, const uint8_t* s2, int n)
{
    while (n-- > 0 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *s1 - *s2;
}

/**
 * @brief Compare first n characters of 2 16-bit strings
 */
uint16_t strncmp16(const uint16_t* s1, const uint16_t* s2, int n)
{
    while (n-- > 0 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *s1 - *s2;
}

/**
 * @brief Compare first n characters of a 16-bit string and an 8-bit string
 */
uint16_t strncmp_16_8(const uint16_t* s1, const uint8_t* s2, int n)
{
    while (n-- > 0 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *s1 - *s2;
}

/**
 * @brief Concatinates two 8-bit integers
 */
uint8_t* strcat(uint8_t* dest, const uint8_t* src)
{
    int i = 0, j = 0;
    while (dest[i])
    {
        i++;
    }
    while ((dest[i++] = src[j++]))
        ;
    return dest;
}

/**
 * @brief Concatinates two 16-bit integers
 */
uint16_t* strcat16(uint16_t* dest, const uint16_t* src)
{
    int i = 0, j = 0;
    while (dest[i])
    {
        i++;
    }
    while ((dest[i++] = src[j++]))
        ;
    return dest;
}

/**
 * @brief Concatinates first n characters of two 8-bit strings
 */
uint8_t* strncat(uint8_t* dest, const uint8_t* src, int n)
{
    int i = 0, j = 0;
    while (dest[i])
    {
        i++;
    }
    while (j < n && src[i])
    {
        dest[i++] = src[j++];
    }
    dest[i] = 0;
    return dest;
}

/**
 * @brief Concatinates first n characters of two 8-bit strings
 */
uint16_t* strncat16(uint16_t* dest, const uint16_t* src, int n)
{
    int i = 0, j = 0;
    while (dest[i])
    {
        i++;
    }
    while (j < n && src[i])
    {
        dest[i++] = src[j++];
    }
    dest[i] = 0;
    return dest;
}

/**
 * @brief Locate first occurance of character in 8-bit string
 */
uint8_t* strchr(const uint8_t* str, uint8_t c)
{
    while (*str)
    {
        if (*str == c)
        {
            return str;
        }
        str++;
    }
    return c == 0 ? str : 0;
}

/**
 * @brief Locate first occurance of character in 16-bit string
 */
uint16_t* strchr16(const uint16_t* str, uint16_t c)
{
    while (*str)
    {
        if (*str == c)
        {
            return str;
        }
        str++;
    }
    return c == 0 ? str : 0;
}

/**
 * @brief Locate last occurance of character in 8-bit string
 */
uint8_t* strrchr(const uint8_t* str, uint8_t c)
{
    const uint8_t* last = 0;
    while (*str)
    {
        if (*str == c)
        {
            last = str;
        }
        str++;
    }
    return c == 0 ? str : last;
}

/**
 * @brief Locate last occurance of character in 16-bit string
 */
uint16_t* strrchr16(const uint16_t* str, uint16_t c)
{
    const uint16_t* last = 0;
    while (*str)
    {
        if (*str == c)
        {
            last = str;
        }
        str++;
    }
    return c == 0 ? str : last;
}

/**
 * @brief Locate substring withing 8-bit string
 */
uint8_t* strstr(const uint8_t* haystack, const uint8_t* needle)
{
    if (!*needle)
    {
        return haystack;
    }

    for (; *haystack; haystack++)
    {
        const uint8_t *h = haystack, *n = needle;
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return haystack;
        }
    }
    return 0;
}

/**
 * @brief Locate substring withing 16-bit string
 */
uint16_t* strstr16(const uint16_t* haystack, const uint16_t* needle)
{
    if (!*needle)
    {
        return haystack;
    }

    for (; *haystack; haystack++)
    {
        const uint16_t *h = haystack, *n = needle;
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return haystack;
        }
    }
    return 0;
}

/**
 * @brief Converts an integer to a string, and copies the results into the buffer
 */
void int_to_cstr(int value, char* buffer)
{
    char temp[12]; // Enough for -2147483648\0
    int i = 0;
    int is_negative = 0;

    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0)
    {
        is_negative = 1;
        // handle INT_MIN carefully
        if (value == -2147483648)
        {
            // special-case: can't negate it in two's complement
            const char* min_str = "-2147483648";
            for (i = 0; min_str[i]; ++i)
                buffer[i] = min_str[i];
            buffer[i] = '\0';
            return;
        }
        value = -value;
    }

    while (value > 0)
    {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative)
    {
        temp[i++] = '-';
    }

    // reverse and copy to buffer
    int j;
    for (j = 0; j < i; ++j)
    {
        buffer[j] = temp[i - j - 1];
    }
    buffer[j] = '\0';
}

/**
 * @brief Copy memory between buffers
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination buffer
 */
void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = dest;
    const unsigned char* s = src;
    for (size_t i = 0; i < n; ++i)
    {
        d[i] = s[i];
    }
    return dest;
}

/**
 * @brief Fill memory with constant byte
 * @param ptr Pointer to memory region
 * @param value Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory region
 */
void* memset(void* ptr, int value, size_t n)
{
    unsigned char* p = ptr;
    for (size_t i = 0; i < n; ++i)
    {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

/**
 * @brief Compare two memory regions
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;

    for (size_t i = 0; i < n; ++i)
    {
        if (p1[i] != p2[i])
        {
            return (int)p1[i] - (int)p2[i];
        }
    }

    return 0;
}