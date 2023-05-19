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

#define PROBE_DATA 0x42

void com_init(uint32_t port) {
    outb(port + LINE_CR, (1 << 7));
    outb(port + 0, 0x0C);
    outb(port + 1, 0x00);
    outb(port + LINE_CR, 0x3);
    outb(port + DATA_REG, PROBE_DATA);

    if (inb(port + DATA_REG) != PROBE_DATA) {
        return;
    }

    outb(port + MODEM_CR, 0x0F); 
}

static int rx_free(port_t port) {
    return (inb(port + LINE_STAT_REG) & (1 << 0));
}

static int tx_free(port_t port) {
    return (inb(port + LINE_STAT_REG) & (1 << 5));
}

uint8_t wait_serial_read(port_t port) {
    while(!rx_free(port));

    return inb(port + DATA_REG);
}

void serial_send(port_t port, uint8_t data) {
    while(!tx_free(port));

    outb(port + DATA_REG, data);
}

void serial_init(void) {
    com_init(COM1);
    com_init(COM2);
}
