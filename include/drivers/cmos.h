#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

enum rtc_registers {
    CMOS_SECONDS         = 0x00,
    CMOS_MINUTES         = 0x02,
    CMOS_HOURS           = 0x04,
    CMOS_WEEKDAY         = 0x06,
    CMOS_DAY_OF_MONTH    = 0x07,
    CMOS_MONTH           = 0x08,
    CMOS_YEAR            = 0x09,
};

void cmos_select_reg(uint8_t reg);

uint8_t cmos_read_data(void);

void cmos_write_data(uint8_t data);
 

#endif
