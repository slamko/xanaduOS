#ifndef PIT_H
#define PIT_H

#include "drivers/int.h"
#include <stddef.h>

typedef uint32_t tick_t;

typedef void (*pit_callback_t)(void);

int pit_init(unsigned long freq);

void sleep_us(uint32_t delay);

void sleep_ms(uint32_t delay);

void sleep_sec(uint32_t delay);

void pit_handler(struct isr_handler_args *args);

void pit_add_callback(pit_callback_t cb, unsigned int priority, size_t period);

#endif
