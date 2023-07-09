#ifndef SERIAL_H
#define SERIAL_H

#include "drivers/int.h"
#include <stdint.h>
#include <stddef.h>

void serial_init(void);

enum COM_Ports {
    COM1 = 0x3f8,
    COM2 = 0x2f8,
    COM3 = 0x3e8,
    COM4 = 0x2E8,
    COM5 = 0x5F8,
    COM6 = 0x4F8,
    COM7 = 0x5E8,
    COM8 = 0x4E8,
};

typedef enum COM_Ports port_t;

void serial_interrupt(struct isr_handler_args *args);

void serial_send(enum COM_Ports port, char data);
char serial_read(enum COM_Ports port);

void serial_write(port_t port, const char *buf, size_t len);

void wait_serial_send(enum COM_Ports port, char data);
uint8_t wait_serial_read(enum COM_Ports port);

int serial_read_buf(port_t port, char *buf, size_t buf_len);

void serial_print(port_t port, char *msg);

#endif

