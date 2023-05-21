#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b 

#define PIC_EOI 0x20
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void pic_mask(uint8_t irq);

void pic_mask_all();

void pic_unmask(uint8_t irq);

void pic_eoi(uint8_t int_id);

void pic_init(uint16_t mask);

#endif
