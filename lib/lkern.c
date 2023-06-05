#include "drivers/fb.h"
#include "lib/kernel.h"

void klog(const char *msg) {
    fb_print_black(msg);
}

void klog_warn(const char *msg) {
    fb_print_black(msg);
}

void klog_error(const char *msg) {
    fb_print_black(msg);
}
