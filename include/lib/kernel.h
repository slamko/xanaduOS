#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define asm_call(body) \
    { \
        asm volatile ("pushal;"); \
        body \
        asm volatile ("popal;"); \
    }

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);

void klog(const char *msg, ...);

void debug_log(const char *msg, ...);

void klog_warn(const char *msg, ...); 

void klog_error(const char *msg, ...); 

void slab_test(void);

#endif
