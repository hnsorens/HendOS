/**
 * @file kstring.h
 * @brief Kernel String Manipulation Interface
 *
 * Provides basic string operations for 8-bit and 16-bit character strings, similar to the standard C library.
 */
#ifndef K_STRING_H
#define K_STRING_H

#include <kint.h>

/**
 * @brief Calculates the length of a null-terminated 8-bit string
 * @param str Poinger to the null-terminated string
 * @return Length of the string in characters
 */
__attribute__((unused)) size_t kernel_strlen(const uint8_t* str);

/**
 * @brief Calcualtes the length of a null-terminated 16-bit string
 * @param str POinter to the null-terminated 16-bit string
 * @return Length of the string in characters
 */
__attribute__((unused)) size_t kernel_strlen16(const uint16_t* str);

/**
 * @brief Copy an 8-bit string from source to destination
 * @param dest Destination buffer
 * @param src Source buffer
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint8_t* kernel_strcpy(uint8_t* dest, const uint8_t* src);

/**
 * @brief Copy an 16-bit string from source to destination
 * @param dest Destination buffer
 * @param src Source buffer
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint16_t* kernel_strcpy16(uint16_t* dest, const uint16_t* src);

/**
 * @brief Copy up to n characters from an 8-bit string
 * @param dest Destination buffer
 * @param src Source Buffer
 * @param n Maximum number of characters to copy
 * @return POinter to destination buffer
 */
__attribute__((unused)) uint8_t* kernel_strncpy(uint8_t* dest, const uint8_t* src, int n);

/**
 * @brief Copy up to n characters from an 16-bit string
 * @param dest Destination buffer
 * @param src Source Buffer
 * @param n Maximum number of characters to copy
 * @return POinter to destination buffer
 */
__attribute__((unused)) uint16_t* kernel_strncpy16(uint16_t* dest, const uint16_t* src, int n);

/**
 * @brief Compare two 8-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint8_t kernel_strcmp(const uint8_t* s1, const uint8_t* s2);

/**
 * @brief Compare two 16-bit strings
 * @param s1 First string to compare1
 * @param s2 Second string to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint16_t kernel_strcmp16(const uint16_t* s1, const uint16_t* s2);

/**
 * @brief Compares a 16-bit string with an 8-bit string
 * @param s1 16-bit string to compare
 * @param s2 8-bit string to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint16_t kernel_strcmp_16_8(const uint16_t* s1, const uint8_t* s2);

/**
 * @brief Compare first n characters of 2 8-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint8_t kernel_strncmp(const uint8_t* s1, const uint8_t* s2, int n);

/**
 * @brief Compare first n characters of 2 16-bit strings
 * @param s1 First string to compare
 * @param s2 Second string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint16_t kernel_strncmp16(const uint16_t* s1, const uint16_t* s2, int n);

/**
 * @brief Compare first n characters of a 16-bit string and an 8-bit string
 * @param s1 16-bit string to compare
 * @param s2 8-bit string to compare
 * @param n Maximum number of characters to compare
 * @return Difference between the first differing characters
 */
__attribute__((unused)) uint16_t kernel_strncmp_16_8(const uint16_t* s1, const uint8_t* s2, int n);

/**
 * @brief Concatinates two 8-bit integers
 * @param dest Destionation buffer containing first string
 * @param src Source buffer to append
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint8_t* kernel_strcat(uint8_t* dest, const uint8_t* src);

/**
 * @brief Concatinates two 16-bit integers
 * @param dest Destionation buffer containing first string
 * @param src Source buffer to append
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint16_t* kernel_strcat16(uint16_t* dest, const uint16_t* src);

/**
 * @brief Concatinates first n characters of two 8-bit strings
 * @param dest Destination buffer containing first string
 * @param src Source buffer to append
 * @param n Maximum number of characters to append
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint8_t* kernel_strncat(uint8_t* dest, const uint8_t* src, int n);

/**
 * @brief Concatinates first n characters of two 8-bit strings
 * @param dest Destination buffer containing first string
 * @param src Source buffer to append
 * @param n Maximum number of characters to append
 * @return Pointer to destination buffer
 */
__attribute__((unused)) uint16_t* kernel_strncat16(uint16_t* dest, const uint16_t* src, int n);

/**
 * @brief Locate first occurance of character in 8-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointer to first occurance or NULL if not found
 */
__attribute__((unused)) uint8_t* kernel_strchr(const uint8_t* str, uint8_t c);

/**
 * @brief Locate first occurance of character in 16-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointer to first occurance or NULL if not found
 */
__attribute__((unused)) uint16_t* kernel_strchr16(const uint16_t* str, uint16_t c);

/**
 * @brief Locate last occurance of character in 8-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointe to last occurance or NULL if not found
 */
__attribute__((unused)) uint8_t* kernel_strrchr(const uint8_t* str, uint8_t c);

/**
 * @brief Locate last occurance of character in 16-bit string
 * @param str String to search
 * @param c Character to locate
 * @return Pointe to last occurance or NULL if not found
 */
__attribute__((unused)) uint16_t* kernel_strrchr16(const uint16_t* str, uint16_t c);

/**
 * @brief Locate substring withing 8-bit string
 * @param haystack String to search
 * @param needle Substring to find
 * @return Pointer to beggining of substring or NULL if not found
 */
__attribute__((unused)) uint8_t* kernel_strstr(const uint8_t* haystack, const uint8_t* needle);

/**
 * @brief Locate substring withing 16-bit string
 * @param haystack String to search
 * @param needle Substring to find
 * @return Pointer to beggining of substring or NULL if not found
 */
__attribute__((unused)) uint16_t* kernel_strstr16(const uint16_t* haystack, const uint16_t* needle);

/**
 * @brief Converts an integer to a string, and copies the results into the buffer
 * @param value number to be converted
 * @param buffer number string dest
 */
__attribute__((unused)) void int_to_cstr(int value, char* buffer);

/**
 * @brief memsey
 */
__attribute__((unused)) void* memset(void* __s, int __c, size_t __n);

#endif /* K_MATH_H */