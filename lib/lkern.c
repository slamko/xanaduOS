#include "drivers/fb.h"
#include "lib/kernel.h"
#include <stdarg.h>

void klog(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fb_vprintf(msg, args);
    va_end(args);
}

void debug_log(const char *msg) {
    fb_print_black(msg);
}

void klog_warn(const char *msg) {
    fb_print_black(msg);
}

void klog_error(const char *msg) {
    fb_print_black(msg);
}
