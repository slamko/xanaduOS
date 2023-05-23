#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_REMAP 0x20
#define PIC2_REMAP 0x28
#define PIC_REMAP PIC1_REMAP

uint16_t pic_get_irr(void);

void pic_mask(uint8_t irq);

void pic_mask_all();

void pic_unmask(uint8_t irq);

void pic_eoi(uint8_t int_id);

void pic_init(uint16_t mask);

#endif
