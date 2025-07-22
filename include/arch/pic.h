/**
 * @file pic.h
 * @brief Programmable Interrupt Controller (PIC) Interface
 *
 * Defines constants and macros for interacting with the legacy x86 PIC.
 */
#ifndef PIC_H
#define PIC_H

/* Define necessary ports and PIC commands */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

#endif /* PIC_H */