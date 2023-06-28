#include "lib/kernel.h"
#include "drivers/fb.h"
#include <stdarg.h>

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
