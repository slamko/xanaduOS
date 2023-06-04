#include "lib/slibc.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/slibc.h"
#include "drivers/serial.h"
#include "lib/kernel.h"
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

enum {
    BAUD_DIVISOR = 0x0C  
};

static char write_buf[1024];
static char read_buf[1024];
static size_t write_pos = 0;
static size_t read_pos = 0;
static size_t cur_pos = 0;

#define PROBE_DATA 0x42
/* #define PROBE_CHECK */

void com_init(uint32_t port) {
    outb(port + LINE_CR, (1 << 7));
    outb(port + 0, BAUD_DIVISOR);
    outb(port + 0, 0x00);
    outb(port + 2, 0xC7);
    outb(port + LINE_CR, 0x3);

#ifdef PROBE_CHECK
    outb(port + DATA_REG, PROBE_DATA);

    if (inb(port + DATA_REG) != PROBE_DATA) {
        return;
    }
#endif

    outb(port + MODEM_CR, 0x0B); 
    outb(port + INT_REG, 0x3);
    /* outb(port + INT_REG, 0x0); */
}

static int rx_ready(port_t port) {
    return (inb(port + LINE_STAT_REG) & (1 << 0));
}

static int tx_free(port_t port) {
    return (inb(port + LINE_STAT_REG) & (1 << 5));
}

void serial_send(port_t port, char data) {
    outb(port + DATA_REG, data);
}

char serial_read(port_t port) {
    return inb(port + DATA_REG);
}

uint8_t wait_serial_read(port_t port) {
    while(!rx_ready(port));

    return serial_read(port);
}

void wait_serial_send(port_t port, char data) {
    while(!tx_free(port));

    serial_send(port, data);
}

void serial_write(port_t port, const char *buf, size_t len) {
    strcpy(write_buf + write_pos, buf, len);
    cur_pos++;
    write_pos += len;
    wait_serial_send(port, buf[0]);
}

void serial_interrupt(struct isr_handler_args args) {
    port_t port = COM1;
    UNUSED(args);
    
    if (tx_free(port) && cur_pos != write_pos) {
        cur_pos++;
        serial_send(port, write_buf[cur_pos - 1]);
    }

    if (rx_ready(port)) {
        read_pos++;
        read_buf[read_pos - 1] = serial_read(port);
    }
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
    /* com_init(COM2); */
    add_irq_handler(COM1_IRQ, &serial_interrupt);

    fb_print_black("Serial port COM1 initialised");
}

