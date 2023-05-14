#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>
#include <stddef.h>

/* #define PACKED __attribute__((packed)) */

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);

#endif
