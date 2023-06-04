#include "drivers/pit.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include <stdint.h>

#define XTAL_FREQ 1193182

unsigned long cnt;

enum {
    SQUARE_WAVE = 0x3,
    LOHI_MODE   = 0x3, 
};

void pit_handler(struct isr_handler_args args) {
    fb_print_black("pit: ");
    fb_print_num(cnt);
    fb_newline();
    cnt++;
}

int pit_init(long freq) {
    if (freq > XTAL_FREQ) {
        return 1;
    }
    
    outb(PIT_MODE_REG, (LOHI_MODE << 4) | (SQUARE_WAVE << 1));
    uint16_t divisor = XTAL_FREQ / freq;

    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    add_irq_handler(IRQ0, &pit_handler);
    return 0;
}
