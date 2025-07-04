#ifndef PIC_H
#define PIC_H

/*
 * Programmable Interrupt Controller (PIC) definitions
 * for the 8259A PIC chips used in x86 systems.
 *
 * The system typically has two PICs cascaded together:
 * - Master PIC (PIC1) handles IRQs 0-7
 * - Slave PIC (PIC2) handles IRQs 8-15
 */

/* Command and Data ports for the Master PIC */
#define PIC1_CMD 0x20  /* Master PIC command port */
#define PIC1_DATA 0x21 /* Master PIC data port */

/* Command and Data ports for the Slave PIC */
#define PIC2_CMD 0xA0  /* Slave PIC command port */
#define PIC2_DATA 0xA1 /* Slave PIC data port */

/* PIC Commands */
#define PIC_EOI 0x20 /* End Of Interrupt command - sent to PIC to acknowledge interrupt handling */

#endif /* PIC_H */