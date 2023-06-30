#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "mem/allocator.h"
#include <stdint.h>

enum {
    CONFIG_ADDRESS          = 0xCF8,
    CONFIG_DATA             = 0xCFC,
    COMMAND_REG_OFFSET      = 0x4,
};

enum {
    RTL_CONFIG_1            = 0x52,
    RTL_MSR                 = 0x58,
    RTL_COMMAND_REG         = 0x37,
    RTL_RX_BUF_START        = 0x30,
    RTL_IMR                 = 0x3C,
    RTL_ISR                 = 0x3E,
};

enum {
    CMD_RST                 = (1 << 4),
};

#define RX_BUF_SIZE 0x2010
static uint32_t *rx_buffer;

uint16_t pci_read_reg(uint32_t bus, uint32_t device, uint32_t function,
                      uint32_t reg_offset) {
    uint32_t address = (1 << 31)
        | reg_offset
        | ((bus & 0xF) << 16)
        | ((device & 0xF) << 11)
        | ((function & 0x3) << 8);

    outl(CONFIG_ADDRESS, address);
}

void rtl_reset(uint16_t io_addr) {
    outb(io_addr + RTL_COMMAND_REG, CMD_RST);
    while(inb(io_addr + RTL_COMMAND_REG) & CMD_RST);
}

void rtl_init_rx_buffer(uint16_t io_addr) {
    uintptr_t buf_phys_addr;
    rx_buffer = kmalloc_phys(RX_BUF_SIZE, &buf_phys_addr);

    outl(io_addr + RTL_RX_BUF_START, buf_phys_addr);
}

void rtl_handler(struct isr_handler_args args) {
    klog("RTL isr\n");
}

void rtl8139_init() {
    uint16_t io_addr = 0;
    outb(io_addr + RTL_CONFIG_1, 0x0);
    
    rtl_reset(io_addr);

    add_irq_handler(IRQ9, &rtl_handler);
    add_irq_handler(IRQ10, &rtl_handler);
    add_irq_handler(IRQ11, &rtl_handler);
}
