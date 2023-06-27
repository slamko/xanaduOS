#include "drivers/pit.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include <stdint.h>

typedef uint32_t tick_t;

static const unsigned long XTAL_FREQ = 1193182;
static tick_t tick;
static unsigned long pit_freq = 10*1000;


enum PIT_channels{
    PIT_CH0         = 0x40,
    PIT_CH1         = 0x41,
    PIT_CH2         = 0x42,
};

typedef enum PIT_channels pit_channel_t;

enum {
    SQUARE_WAVE     = 0x6,
    LOHI_MODE       = 0x30, 
    PIT_MODE_REG    = 0x43,
};

void pit_handler(struct isr_handler_args args) {
    tick++;
}

uint16_t get_pit_count(pit_channel_t ch) {
    outb(PIT_MODE_REG, (ch & 0x0F) << 6);

    uint16_t count = inb(ch);
    count |= (inb(ch) << 8);
    return count;
}

void wait_ticks(tick_t delay) {
    tick_t init_tick = tick;

    if (init_tick + delay < tick) {
        // overflow

        while(tick >= init_tick) {
            halt();
        }
        delay -= (UINT32_MAX - init_tick);
        init_tick = tick;
    }

     while (tick < init_tick + delay) {
        halt();
    }
}

void sleep_us(uint32_t delay) {
    if (!delay) {
        return;
    }
    
    unsigned long min_delay = (1000*1000) / pit_freq;
    min_delay *= 2; // for slow timers

    if (delay < min_delay) {
        delay = min_delay;
    }
    
    tick_t delay_ticks = 0;

    if (delay < 1000) {
        delay_ticks = delay * pit_freq;
        delay_ticks /= (1000 * 1000);
    } else if (delay < (1000*1000)){
        delay_ticks = ((delay / 1000) * pit_freq) / 1000;
    } else {
        delay_ticks = (delay / (1000 * 1000)) * pit_freq;
    }

    delay_ticks /= 2; // for slow timers (like in qemu)

    wait_ticks(delay_ticks);
}

void sleep_ms(uint32_t delay) {
    if (!delay) {
        return;
    }

    tick_t delay_ticks = (delay * pit_freq) / 1000;

    if (delay_ticks == 1) {
        sleep_us(delay * 1000);
        return;
    }

    delay_ticks /= 2; // for slow timers (like in qemu)

    wait_ticks(delay_ticks);
}

void sleep_sec(uint32_t delay) {
    if (!delay) {
        return;
    }

    tick_t delay_ticks = (delay * pit_freq);
 
    if (delay_ticks == 1) {
        sleep_ms(delay * 1000);
        return;
    }
   
    delay_ticks /= 2; // for slow timers (like in qemu)
    wait_ticks(delay_ticks);
}

int pit_init(unsigned long freq) {
    if (freq > XTAL_FREQ) {
        return 1;
    }

    uint32_t cflags = disable_int();

    if (freq) {
        pit_freq = freq;
    } else {
        freq = pit_freq;
    }

    outb(PIT_MODE_REG, LOHI_MODE | SQUARE_WAVE);
    io_wait();
    uint16_t divisor = XTAL_FREQ / freq;

    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    io_wait();

    add_irq_handler(IRQ0, &pit_handler);
    recover_int(cflags);

    klog("PIT initialized\n");
    
    return 0;
}
