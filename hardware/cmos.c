#include <stdint.h>

#include "drivers/cmos.h"
#include "drivers/pit.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

enum cmos {
    CMOS_COMMAND_PORT = 0x70,
    CMOS_DATA_PORT    = 0x71,
};

void cmos_select_reg(uint8_t reg) {
    outb(CMOS_COMMAND_PORT, reg);
    sleep_ms(50);
}

uint8_t cmos_read_data(void) {
    return inb(CMOS_DATA_PORT);
}

void cmos_write_data(uint8_t data) {
    outb(CMOS_DATA_PORT, data);
    io_wait();
}
