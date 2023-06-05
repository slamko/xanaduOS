#ifndef PIC_H
#define PIC_H

#include <stdint.h>
#include <stdbool.h>

#define PIC1_REMAP 0x20
#define PIC2_REMAP 0x28
#define PIC_REMAP PIC1_REMAP

enum {
    IRQ0 = 0,
    IRQ1,
    IRQ2,
    IRQ3,
    IRQ4,
    IRQ5,
    IRQ6,
    IRQ7,
    IRQ8,
    IRQ9,
    IRQ10,
    IRQ11,
    IRQ12,
    IRQ13,
    IRQ14,
    IRQ15,
};

uint16_t pic_get_irr(void);

void pic_mask(uint8_t irq);

void pic_mask_all();

void pic_unmask(uint8_t irq);

void pic_eoi(uint8_t int_id);

void pic_init(uint16_t mask, bool auto_eoi);

#endif
