#include "drivers/pit.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include <stdint.h>

static const unsigned long XTAL_FREQ = 1193182;
static unsigned long tick;
static unsigned long pit_freq = XTAL_FREQ;

enum PIT_channels{
    PIT_CH0         = 0x40,
    PIT_CH1         = 0x41,
    PIT_CH2         = 0x42,
};

enum {
    SQUARE_WAVE     = 0x3,
    LOHI_MODE       = 0x30, 
    PIT_MODE_REG    = 0x43,
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
}

uint16_t get_pit_count(enum PIT_channels ch) {
    outb(PIT_MODE_REG, (ch & 0x0F) << 6);

    uint16_t count = inb(ch);
    count |= inb(ch) << 8;
    return count;
}

int pit_init(unsigned long freq) {
    if (freq > XTAL_FREQ) {
        return 1;
    }

    cli();

    if (freq) {
        pit_freq = freq;
    } else {
        freq = pit_freq;
    }

    outb(PIT_MODE_REG, LOHI_MODE | (SQUARE_WAVE << 1));
    uint16_t divisor = XTAL_FREQ / freq;

    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    io_wait();

    add_irq_handler(IRQ0, &pit_handler);
    asm volatile("sti");

    fb_newline();
    fb_print_black("PIT initialized");
    
    return 0;
}
