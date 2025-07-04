/**
 * @file io.c
 * @brief Hardware Port I/O Implementation
 *
 * Implements x86 port I/O operations using inline assembly.
 */

#include <arch/io.h>

/**
 * @brief Write a byte to a hardware port
 * @param port The I/O port address
 * @param value The 8-bit value to write
 */
void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read a byte from a hardware port
 * @param port The I/O port address
 * @return The 8-bit value read from the port
 */
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Read a word (16-bit) from a hardware port
 * @param port The I/O port address
 * @return The 16-bit value read from the port
 */
uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Write a word (16-bit) to a hardware port
 * @param port The I/O port address
 * @param value The 16-bit value to write
 */
void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}