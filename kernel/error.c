#include "kernel/error.h"
#include "lib/kernel.h"

void reboot() {
}

int panic(const char *msg, struct error_state stat) {
    klog_error(msg);
    reboot();

    return 0;
}
