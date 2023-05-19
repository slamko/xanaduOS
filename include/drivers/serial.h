#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

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

void serial_send(enum COM_Ports port, uint8_t data);

uint8_t wait_serial_read(enum COM_Ports port);

#endif

