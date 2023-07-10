#include "drivers/pit.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "mem/allocator.h"
#include <stddef.h>
#include <stdint.h>

typedef uint32_t tick_t;

static const unsigned long XTAL_FREQ = 1193182;
static volatile tick_t tick;
static unsigned long pit_freq = 10*1000;

enum PIT_channels{
    PIT_CH0         = 0x40,
    PIT_CH1         = 0x41,
    PIT_CH2         = 0x42,
};

typedef enum PIT_channels pit_channel_t;

struct pit_event {
    pit_callback_t cb;
    unsigned int priority;
    size_t period_ms;
    tick_t last_call_tick;

    struct pit_event *next;
    struct pit_event *next_in_queue;
};

struct pit_event *events;
struct pit_event *event_queue;

enum {
    SQUARE_WAVE     = 0x6,
    LOHI_MODE       = 0x30, 
    PIT_MODE_REG    = 0x43,
};

void insert_invent_in_queue(struct pit_event *event) {
    struct pit_event *next = event_queue;
    event_queue = event;
    event->next_in_queue = next;
}

void remove_event_from_queue(struct pit_event *event) {
    event_queue = event->next_in_queue;
}

void pit_handler(struct isr_handler_args *args) {
    (void)args;
    struct pit_event *event = events;

    foreach(event,
            if (!event->cb) continue;

            if(tick - event->last_call_tick > event->period_ms * 100) {
                /* insert_invent_in_queue(event); */
                event->cb(args);
                event->last_call_tick = tick;
            }
        );
    
    tick += 2;
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

void pit_add_callback(pit_callback_t cb, unsigned int priority, size_t period) {
    struct pit_event *new_event;
    new_event = kmalloc(sizeof *new_event);
    new_event->cb = cb;
    new_event->last_call_tick = 0;
    new_event->next_in_queue = NULL;
    new_event->period_ms = period;
    new_event->priority = priority;

    single_ll_insert(events, new_event);
}

void pit_event_loop(void) {
    struct pit_event *eiq = event_queue;

    for(; eiq; eiq = eiq->next_in_queue) {
        /* eiq->cb(a); */
        /* remove_event_from_queue(eiq); */
    }
}

void sleep_us(uint32_t delay) {
    if (!delay) {
        return;
    }
    
    unsigned long min_delay = (1000*1000) / pit_freq;
    /* min_delay *= 2; // for slow timers */

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

    /* delay_ticks /= 2; // for slow timers (like in qemu) */

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

    /* delay_ticks /= 2; // for slow timers (like in qemu) */

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
   
    /* delay_ticks /= 2; // for slow timers (like in qemu) */
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
