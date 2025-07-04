/**
 * @file io.h
 * @brief Hardware Port I/O Interface
 *
 * Provides functions for reading/writing to hardware ports
 * using x86 in/out instructions.
 */

#ifndef IO_H
#define IO_H

#include <kint.h>

/**
 * @brief Write a byte to a hardware port
 * @param port The I/O port address
 * @param value The 8-bit value to write
 */
void outb(uint16_t port, uint8_t value);

/**
 * @brief Read a byte from a hardware port
 * @param port The I/O port address
 * @return The 8-bit value read from the port
 */
uint8_t inb(uint16_t port);

/**
 * @brief Read a word (16-bit) from a hardware port
 * @param port The I/O port address
 * @return The 16-bit value read from the port
 */
uint16_t inw(uint16_t port);

/**
 * @brief Write a word (16-bit) to a hardware port
 * @param port The I/O port address
 * @param value The 16-bit value to write
 */
void outw(uint16_t port, uint16_t value);

#endif /* IO_H */