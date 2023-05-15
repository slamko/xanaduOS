#include "lib/slibc.h"
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

int strcmp(const char *str, const char *cmp) {
    if (!str || !cmp) return -1;
    int diff = 0;
    for(size_t i = 0; str[i] && cmp[i]; diff += str[i] - cmp[i]);
    return diff;
}

size_t strlen(const char *str) {
    size_t i;
    for (i = 0; str[i] != 0; i++);
    return i;
}

int strncmp(const char *str, const char *cmp, size_t len) {
    if (!str || !cmp) return -1;
    int diff = 0;
    for(size_t i = 0; i < len && str[i] && cmp[i]; i++) {
        diff += str[i] - cmp[i];
    }
    return diff;
}

size_t strnlen(const char *str, size_t len) {
    size_t i;
    for (i = 0; i < len && str[i] != 0; i++);
    return i;
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" 
                  : "=a" (ret) 
                  : "dN" (port));
    return ret;
}

inline void io_wait(void)
{
    outb(0x80, 0);
}

