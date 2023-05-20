#include "lib/slibc.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "drivers/serial.h"
#include "lib/typedef.h"
#include <stddef.h>
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
#define PROBE_CHECK

void com_init(uint32_t port) {
    outb(port + LINE_CR, (1 << 7));
    outb(port + 0, 0x0C);
    outb(port + 1, 0x00);
    outb(port + LINE_CR, 0x3);

#ifdef PROBE_CHECK
    outb(port + DATA_REG, PROBE_DATA);

    if (inb(port + DATA_REG) != PROBE_DATA) {
        return;
    }
#endif

    outb(port + MODEM_CR, 0x0F); 
    outb(port + INT_REG, 0x3);
    /* outb(port + INT_REG, 0x0); */
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

void serial_interrupt() {
    outb(COM1 + DATA_REG, 'a');
}

void serial_send(port_t port, uint8_t data) {
    while(!tx_free(port));

    outb(port + DATA_REG, data);
}

void serial_print(port_t port, char *msg) {
    for (size_t i = 0; msg[i]; i++) {
        serial_send(port, msg[i]);
    }
}

int serial_read_buf(port_t port, char *buf, size_t buf_len) {
    for (size_t i = 0; i < buf_len - 1; i++) {
        buf[i] = wait_serial_read(port);

        if (buf[i] == '\n' || buf[i] == '\r') {
            buf[i + 1] = 0;
            return 0;
        }
    }

    buf[buf_len - 1] = 0;
    return 1;
}

void serial_init(void) {
    com_init(COM1);
}

