#include "lib/typedef.h"
#include <stddef.h>

void *memset(void *buf, int val, size_t siz) {
    char *bbuf = (char *) buf;
    for (size_t i = 0; i < siz; bbuf[i++] = val);
    return buf;
}

