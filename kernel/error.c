#include "kernel/error.h"
#include "drivers/fb.h"
#include "lib/kernel.h"
#include "drivers/int.h"
#include "drivers/mouse.h"
#include <stdint.h>

__attribute__((weak)) void reboot() {
    uint8_t temp;
    cli();

    temp = ps2_read_status();
    while (temp & 1) {
        temp = ps2_read_data();
    }

    ps2_write_cmd(PS2_RESET);
    
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
