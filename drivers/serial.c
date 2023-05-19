#include "lib/slibc.h"
#include "lib/slibc.h"
#include "drivers/serial.h"
#include "lib/typedef.h"
#include <stdint.h>

enum COM_Registers {
    DATA_REG = 0,
    INT_REG = 1,
    LINE_CR = 3,
    MODEM_CR = 4,
    LINE_STAT_REG = 5,
    MODEM_STAT_REG = 6,
};

void com_init(uint32_t port) {
    outb(port + LINE_CR, (1 << 7));
    outb(port + 0, 0x0C);
    outb(port + 1, 0x00);
    outb(port + LINE_CR, 0x3);
    outb(port + DATA_REG, 0x42);

    if (inb(port + DATA_REG) != 0x42) {
        return;
    }

    outb(port + MODEM_CR, 0x0F); 
}

uint8_t wait_serial_read(enum COM_Ports port) {
    while(!(inb(port + LINE_STAT_REG) & 1));

    return inb(port + DATA_REG);
}

void serial_send(enum COM_Ports port, uint8_t data) {
    while(!(inb(port + LINE_STAT_REG) & (1<<5)));

    outb(port + DATA_REG, data);
}

void serial_init(void) {
    com_init(COM1);
}
