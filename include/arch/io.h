// io.h

#ifndef IO_H
#define IO_H

#include <kint.h>

void outb(uint16_t port, uint8_t value);

uint8_t inb(uint16_t port);

uint16_t inw(uint16_t port);

void outw(uint16_t port, uint16_t value);

#endif