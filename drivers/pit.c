#include "drivers/pit.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include <stdint.h>

#define XTAL_FREQ 1193182

static unsigned long tick;
static unsigned long pit_freq = XTAL_FREQ;

enum {
    SQUARE_WAVE = 0x3,
    LOHI_MODE   = 0x3, 
};

void sleep_ms(unsigned long delay) {
    unsigned long init_tick = tick;
    fb_print_num(init_tick);
    unsigned long delay_ticks = (delay * pit_freq) / 1000;

    while (tick < init_tick + delay_ticks) {
        halt();
    }
}

void pit_handler(struct isr_handler_args args) {
    tick++;
    if (tick > 100) {
        fb_newline();
        fb_print_num(tick);
    }
}

int pit_init(unsigned long freq) {
    if (freq > XTAL_FREQ) {
        return 1;
    }

    if (freq) {
        pit_freq = freq;
    } else {
        freq = pit_freq;
    }
    
    add_irq_handler(IRQ0, &pit_handler);

    outb(PIT_MODE_REG, (LOHI_MODE << 4) | (SQUARE_WAVE << 1));
    uint16_t divisor = XTAL_FREQ / freq;

    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    return 0;
}
