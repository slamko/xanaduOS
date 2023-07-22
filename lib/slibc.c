#include "lib/slibc.h"
#include "drivers/fb.h"
#include "lib/kernel.h"
#include <stddef.h>
#include <stdint.h>

int atoi(const char *str, size_t len, int radix) {
    unsigned int order = 1;
    int res = 0;
    
    for (unsigned int i = 12; i > 0; i--) {
        if (str[i - 1]) {
            res += (str[i - 1] - '0') * order;
            order *= radix;
        }
    }

    return res;
}

void *memset(void *buf, int val, size_t siz) {
    char *bbuf = (char *) buf;
    for (size_t i = 0; i < siz; bbuf[i++] = val);
    return buf;
}

void *memcpy(void *buf, const void *cpy, size_t len) {
    if (!buf || !cpy) return NULL;
    
    unsigned char *dest = (unsigned char *)buf;
    unsigned char *copy = (unsigned char *)cpy;
    
    for (size_t i = 0; i < len; i++) {
        dest[i] = copy[i];
    }

    return dest;
}

int strcmp(const char *str, const char *cmp) {
    if (!str || !cmp) return -1;

    int diff = 0;

    for(size_t i = 0; ; i++) {
        diff += str[i] - cmp[i];
        if (!str[i] || !cmp[i]) {
            return diff;
        }
    }

    return diff;
}

char *strcpy(char *str, const char *cpy, size_t len) {
    if (!str || !cpy) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        str[i] = cpy[i];
    }

    return str;
}

size_t strnlen(const char *str, size_t len) {
    size_t i;
    for (i = 0; str[i] != 0 && i < len; i++);
    return i;
}

size_t strlen(const char *str) {
    size_t i;
    for (i = 0; str[i] != 0; i++);
    return i;
}

int strneq(const char *str, const char *cmp, size_t len) {
    if (!str || !cmp) return -1;
    if (len == 0) return 0;

    for(size_t i = 0; i < len && (str[i] || cmp[i]); i++) {
        if (str[i] != cmp[i]) {
            return 1;
        }
    }

    return 0;
}

inline void io_wait(void)
{
    outb(0x80, 0);
}

