#ifndef DEV_H
#define DEV_H

#include <stddef.h>
#include <stdint.h>

struct char_buffer {
    char *buf;
    void (*flush)(void);
    size_t pos;
    size_t size;
};

#endif
