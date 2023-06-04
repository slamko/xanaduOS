#ifndef PIT_H
#define PIT_H

#include "drivers/int.h"

enum {
    PIT_CH0         = 0x40,
    PIT_MODE_REG    = 0x43,
};

int pit_init(long freq);

void pit_handler(struct isr_handler_args);
#endif
