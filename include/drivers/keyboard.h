#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "drivers/int.h"
#include <stdint.h>

#define KBD_INPUT_PORT 0x60
#define KBD_STATUS_PORT 0x64

enum {
    KBD_INT_REC_NUM      = 256,
    KBD_DEL              = 127,
    KBD_LAYOUT_MAX_CODE  = 512,
};

enum SPEC_CODES {
    UP_ARROW = 344,
    DOWN_ARROW = 345
};

typedef int (*receiver)(uint32_t);

void kbd_interrupt(struct isr_handler_args);
void kbd_add_receiver(receiver f);

void kbd_init(void);
uint32_t kbd_read(void);


extern receiver receiver_f[KBD_INT_REC_NUM];

#endif
