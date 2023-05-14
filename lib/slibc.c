#include "lib/typedef.h"
#include <stddef.h>
#include <stdint.h>

void *memset(void *buf, int val, size_t siz) {
    char *bbuf = (char *) buf;
    for (size_t i = 0; i < siz; bbuf[i++] = val);
    return buf;
}

void outb(uint16_t port, uint8_t value) {
    asm volatile ("out %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" 
                  : "=a" (ret) 
                  : "dN" (port));
    return ret;
}

