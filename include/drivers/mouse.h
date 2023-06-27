#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

enum {
    PS2_ENABLE_AUX          = 0xA8,
    PS2_ACKNOWLEDGE         = 0xFA,
    PS2_GET_COMPAQ_STAT     = 0x20,
    PS2_SET_COMPAQ_STAT     = 0x60,
    PS2_SET_DEFAULTS        = 0xF6,
    PS2_ENABLE_STREAMING    = 0xF4,
    PS2_RESET               = 0xFE,
};


uint8_t ps2_read_data(void);

uint8_t ps2_read_status(void);

void ps2_write_cmd(uint8_t cmd);
    
int ps2_init(void);

#endif
