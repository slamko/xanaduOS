#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

/* #define PACKED __attribute__((packed)) */

void debug_log(const char *msg);

void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);

void klog(const char *msg, ...);

void klog_warn(const char *msg); 

void klog_error(const char *msg); 

#endif
