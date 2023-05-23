#include <stdint.h>

#include "drivers/pic.h"
#include "drivers/int.h"
#include "lib/slibc.h"
#include "lib/typedef.h"

#define PIC1            0x20
#define PIC2            0xA0
#define PIC1_COMMAND	PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI         0x20

enum INIT_CW1 {
    ICW4_INIT       = (1 << 0),
    SINGLE_MODE     = (1 << 1),
    LEVEL_TRIG_MODE = (1 << 2),
    ICW_INIT        = (1 << 4),
};

enum INIT_CW3 {
    SLAVE_PIN_IR    = (1 << 2),
    SLAVE_ID        = 0x2,
};

enum INIT_CW4 {
    X8086_MODE      = (1 << 0),
    AUTO_EOI        = (1 << 1),
};

enum {
    OCW3_CMD        = (1 << 3),
    OCW3_READ_ISR   = (3 << 0),
    OCW3_READ_IRR   = (1 << 1),
};

#define ICW1 (((ICW_INIT | ICW4_INIT) & ~SINGLE_MODE) & ~LEVEL_TRIG_MODE)

static inline void outb_wait(uint16_t port, uint8_t value) {
    outb(port, value);
    io_wait();
}

static void pic_remap(uint16_t master_offset, uint16_t slave_offset) {
    uint8_t master_mask, slave_mask;

    master_mask = inb(PIC1_DATA);
    slave_mask = inb(PIC2_DATA);

    outb_wait(PIC1_COMMAND, ICW1);
    outb_wait(PIC2_COMMAND, ICW1);

    outb_wait(PIC1_DATA, master_offset);
    outb_wait(PIC2_DATA, slave_offset);

    outb_wait(PIC1_DATA, SLAVE_PIN_IR);
    outb_wait(PIC2_DATA, SLAVE_ID);

    outb_wait(PIC1_DATA, X8086_MODE);
    outb_wait(PIC2_DATA, X8086_MODE);
    
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

static uint16_t pic_get_irq_reg(int ocw3)
{
    outb(PIC1_COMMAND, OCW3_CMD | ocw3);
    outb(PIC2_COMMAND, OCW3_CMD | ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}
 
uint16_t pic_get_irr(void)
{
    return pic_get_irq_reg(OCW3_READ_IRR);
}
 
uint16_t pic_get_isr(void)
{
    return pic_get_irq_reg(OCW3_READ_ISR);
}

void pic_mask(uint8_t irq) {
    uint16_t port = PIC1_DATA;
    uint8_t value;

    if (irq >= 8) {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

static void pic_mask_set(uint16_t mask) {
    uint8_t mask_low = mask & 0xFF;
    uint8_t mask_high = (mask >> 8) & 0xFF;

    outb(PIC1_DATA, mask_low);
    outb(PIC2_DATA, mask_high);
}

static void pic_mask_clear(uint16_t mask) {
    uint8_t mask_low = ~(mask & 0xFF);
    uint8_t mask_high = ~((mask >> 8) & 0xFF);

    outb(PIC1_DATA, mask_low);
    outb(PIC2_DATA, mask_high);
}

void pic_mask_all() {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void pic_unmask(uint8_t irq) {
    uint16_t port = PIC1_DATA;
    uint8_t value;

    if (irq >= 8) {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_eoi(uint8_t int_id) {
    if (int_id >= PIC2_REMAP) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_init(uint16_t mask) {
    pic_remap(PIC1_REMAP, PIC2_REMAP);

    pic_mask_all();
    pic_mask_clear(mask);
}
