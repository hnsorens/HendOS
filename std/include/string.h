/* string.h */
#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <types.h>

/**
 * @file string.h
 * @brief String manipulation functions
 *
 * This module provides basic string operations similar to standard C library
 * but with support of both 8-bit and 16-bit character strings
 */

/**
 * @brief Calculates the length of a null-terminated 8-bit string
 * @param str Poinger to the null-terminated string
 * @return Length of the string in characters
 */
size_t strlen(const uint8_t* str);

/**
 * @brief Calcualtes the length of a null-terminated 16-bit string
 * @param str POinter to the null-terminated 16-bit string
 * @return Length of the string in characters
 */
size_t strlen16(const uint16_t* str);

/**
 * @brief Copy an 8-bit string from source to destination
 * @param dest Destination buffer
 * @param src Source buffer
 * @return Pointer to destination buffer
 */
uint8_t* strcpy(uint8_t* dest, const uint8_t* src);

/**
 * @brief Copy an 16-bit string from source to destination
 * @param dest Destination buffer
 * @param src Source buffer
 * @return Pointer to destination buffer
 */
uint16_t* strcpy16(uint16_t* dest, const uint16_t* src);

/**
 * @brief Copy up to n characters from an 8-bit string
 * @param dest Destination buffer
 * @param src Source Buffer
 * @param n Maximum number of characters to copy
 * @return POinter to destination buffer
 */
uint8_t* strncpy(uint8_t* dest, const uint8_t* src, int n);

/**
 * @brief Copy up to n characters from an 16-bit string
 * @param dest Destination buffer
 * @param src Source Buffer
 * @param n Maximum number of characters to copy
 * @return POinter to destination buffer
 */
uint16_t* strncpy16(uint16_t* dest, const uint16_t* src, int n);

/**
 * @brief Compare two 8-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @return Difference between the first differing characters
 */
uint8_t strcmp(const uint8_t* s1, const uint8_t* s2);

/**
 * @brief Compare two 16-bit strings
 * @param s1 First string to compare1
 * @param s2 Second string to compare
 * @return Difference between the first differing characters
 */
uint16_t strcmp16(const uint16_t* s1, const uint16_t* s2);

/**
 * @brief Compares a 16-bit string with an 8-bit string
 * @param s1 16-bit string to compare
 * @param s2 8-bit string to compare
 * @return Difference between the first differing characters
 */
uint16_t strcmp_16_8(const uint16_t* s1, const uint8_t* s2);

/**
 * @brief Compare first n characters of 2 8-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
uint8_t strncmp(const uint8_t* s1, const uint8_t* s2, int n);

/**
 * @brief Compare first n characters of 2 16-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
uint16_t strncmp16(const uint16_t* s1, const uint16_t* s2, int n);

/**
 * @brief Compare first n characters of a 16-bit string and an 8-bit string
 * @param s1 16-bit string to compare
 * @param s2 8-bit string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
uint16_t strncmp_16_8(const uint16_t* s1, const uint8_t* s2, int n);

/**
 * @brief Concatinates two 8-bit integers
 * @param dest Destionation buffer containing first string
 * @param src Source buffer to append
 * @return Pointer to destination buffer
 */
uint8_t* strcat(uint8_t* dest, const uint8_t* src);

/**
 * @brief Concatinates two 16-bit integers
 * @param dest Destionation buffer containing first string
 * @param src Source buffer to append
 * @return Pointer to destination buffer
 */
uint16_t* strcat16(uint16_t* dest, const uint16_t* src);

/**
 * @brief Concatinates first n characters of two 8-bit strings
 * @param dest Destination buffer containing first string
 * @param src Source buffer to append
 * @param n Maximum number of characters to append
 * @return Pointer to destination buffer
 */
uint8_t* strncat(uint8_t* dest, const uint8_t* src, int n);

/**
 * @brief Concatinates first n characters of two 8-bit strings
 * @param dest Destination buffer containing first string
 * @param src Source buffer to append
 * @param n Maximum number of characters to append
 * @return Pointer to destination buffer
 */
uint16_t* strncat16(uint16_t* dest, const uint16_t* src, int n);

/**
 * @brief Locate first occurance of character in 8-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointer to first occurance or NULL if not found
 */
uint8_t* strchr(const uint8_t* str, uint8_t c);

/**
 * @brief Locate first occurance of character in 16-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointer to first occurance or NULL if not found
 */
uint16_t* strchr16(const uint16_t* str, uint16_t c);

/**
 * @brief Locate last occurance of character in 8-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointe to last occurance or NULL if not found
 */
uint8_t* strrchr(const uint8_t* str, uint8_t c);

/**
 * @brief Locate last occurance of character in 16-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointe to last occurance or NULL if not found
 */
uint16_t* strrchr16(const uint16_t* str, uint16_t c);

/**
 * @brief Locate substring withing 8-bit string
 * @param haystack String to search
 * @param needle Substring to find
 * @return Pointer to beggining of substring or NULL if not found
 */
uint8_t* strstr(const uint8_t* haystack, const uint8_t* needle);

/**
 * @brief Locate substring withing 16-bit string
 * @param haystack String to search
 * @param needle Substring to find
 * @return Pointer to beggining of substring or NULL if not found
 */
uint16_t* strstr16(const uint16_t* haystack, const uint16_t* needle);

/**
 * @brief Converts an integer to a string, and copies the results into the buffer
 * @param value number to be converted
 * @param buffer number string dest
 */
void int_to_cstr(int value, char* buffer);

/**
 * @brief Copy memory between buffers
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination buffer
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * @brief Fill memory with constant byte
 * @param ptr Pointer to memory region
 * @param value Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory region
 */
void* memset(void* ptr, int value, size_t n);

/**
 * @brief Compare two memory regions
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void* s1, const void* s2, size_t n);

#endif /* STRING_H */