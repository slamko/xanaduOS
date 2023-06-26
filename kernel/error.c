#include "kernel/error.h"
#include "drivers/fb.h"
#include "lib/kernel.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include <stdint.h>

__attribute__((weak)) void reboot() {
    uint8_t temp;
    cli();

    temp = inb(KBD_STATUS_PORT);
    while (temp & 1) {
        temp = inb(KBD_INPUT_PORT);
    }
        
    outb(KBD_STATUS_PORT, 0xFE);

    while(1) {
        halt();
    }
}

// non fatal error occured
void error(const char *msg, struct error_state stat) {
    klog_error(msg);
    klog_error("Error type: ");
    fb_print_num(stat.err);
}

// fatal error occured. Can not continue
int panic(const char *msg, struct error_state stat) {
    klog_error(msg);
    reboot();

    return 0;
}
