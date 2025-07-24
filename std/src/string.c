/* kstring.c */
#include <string.h>

size_t strlen(const char* __s)
{
    size_t len = 0;
    while (__s[len])
    {
        len++;
    }
    return len;
}

char* strcpy(char* __restrict __dest, const char* __restrict __src)
{
    size_t i = 0;
    while ((__dest[i] = __src[i]))
    {
        i++;
    }
    return __dest;
}

char* strncpy(char* __restrict __dest, const char* __restrict __src, size_t __n)
{
    size_t i = 0;
    while (i < __n && __src[i])
    {
        __dest[i] = __src[i];
        i++;
    }
    while (i < __n)
    {
        __dest[i++] = '\0';
    }
    return __dest;
}

int strcmp(const char* __s1, const char* __s2)
{
    while (*__s1 && (*__s1 == *__s2))
    {
        __s1++;
        __s2++;
    }
    return *__s1 - *__s2;
}

int strncmp(const char* __s1, const char* __s2, size_t __n)
{
    while (__n-- > 0 && *__s1 && (*__s1 == *__s2))
    {
        __s1++;
        __s2++;
    }
    return __n < 0 ? 0 : *__s1 - *__s2;
}

char* strcat(char* __restrict __dest, const char* __restrict __src)
{
    int i = 0, j = 0;
    while (__dest[i])
    {
        i++;
    }
    while ((__dest[i++] = __src[j++]))
        ;
    return __dest;
}

char* strncat(char* __restrict __dest, const char* __restrict __src, size_t __n)
{
    int i = 0, j = 0;
    while (__dest[i])
    {
        i++;
    }
    while (j < __n && __src[i])
    {
        __dest[i++] = __src[j++];
    }
    __dest[i] = 0;
    return __dest;
}

char* strchr(const char* __s, int __c)
{
    while (*__s)
    {
        if (*__s == __c)
        {
            return __s;
        }
        __s++;
    }
    return __c == 0 ? __s : 0;
}

char* strrchr(const char* __s, int __c)
{
    const char* last = 0;
    while (*__s)
    {
        if (*__s == __c)
        {
            last = __s;
        }
        __s++;
    }
    return __c == 0 ? __s : last;
}

char* strstr(const char* __haystack, const char* __needle)
{
    if (!*__needle)
    {
        return __haystack;
    }

    for (; *__haystack; __haystack++)
    {
        const unsigned char *h = __haystack, *n = __needle;
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return __haystack;
        }
    }
    return 0;
}

void* memcpy(void* __restrict __dest, const void* __restrict __src, size_t __n)
{
    unsigned char* d = __dest;
    const unsigned char* s = __src;
    for (size_t i = 0; i < __n; ++i)
    {
        d[i] = s[i];
    }
    return __dest;
}

void* memset(void* __s, int __c, size_t __n)
{
    unsigned char* p = __s;
    for (size_t i = 0; i < __n; ++i)
    {
        p[i] = (unsigned char)__c;
    }
    return __s;
}

int memcmp(const void* __s1, const void* __s2, size_t __n)
{
    const unsigned char* p1 = (const unsigned char*)__s1;
    const unsigned char* p2 = (const unsigned char*)__s2;

    for (size_t i = 0; i < __n; ++i)
    {
        if (p1[i] != p2[i])
        {
            return (int)p1[i] - (int)p2[i];
        }
    }

    return 0;
}

void* memmove(void* __dest, const void* __src, size_t __n)
{
    unsigned char* d = __dest;
    const unsigned char* s = __src;
    if (d < s)
    {
        for (size_t i = 0; i < __n; ++i)
        {
            d[i] = s[i];
        }
    }
    else if (d > s)
    {
        for (size_t i = __n; i > 0; --i)
        {
            d[i - 1] = s[i - 1];
        }
    }
    return __dest;
}

void* memchr(const void* __s, int __c, size_t __n)
{
    const unsigned char* p = __s;
    for (size_t i = 0; i < __n; ++i)
    {
        if (p[i] == (unsigned char)__c)
        {
            return (void*)(p + i);
        }
    }
    return NULL;
}

size_t strspn(const char* __s, const char* __accept)
{
    size_t len = 0;
    while (__s[len] && strchr(__accept, __s[len]))
    {
        len++;
    }
    return len;
}

size_t strcspn(const char* __s, const char* __reject)
{
    size_t len = 0;
    while (__s[len] && !strchr(__reject, __s[len]))
    {
        len++;
    }
    return len;
}

char* strpbrk(const char* __s, const char* __accept)
{
    while (*__s)
    {
        if (strchr(__accept, *__s))
        {
            return (char*)__s;
        }
        __s++;
    }
    return NULL;
}