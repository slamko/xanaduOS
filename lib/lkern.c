#include "lib/kernel.h"
#include "drivers/fb.h"
#include <stdarg.h>

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void klog(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fb_vprintf(msg, args);
    va_end(args);
}

void debug_log(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fb_vprintf(msg, args);
    va_end(args);
}
void klog_warn(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fb_vprintf_color(msg, FB_LIGHT_CYAN, FB_BLACK, args);
    va_end(args);
}

void klog_error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fb_vprintf_color(msg, FB_LIGHT_RED, FB_BLACK, args);
    va_end(args);
}
