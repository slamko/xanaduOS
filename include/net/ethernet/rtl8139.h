#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void rtl8139_init(uint8_t bus, uint8_t dev_num, uint8_t irq, uint16_t io_base);

#endif
