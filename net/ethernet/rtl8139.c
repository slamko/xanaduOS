#include "net/ethernet/rtl8139.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "mem/allocator.h"
#include "ipc/pci.h"
#include <stdint.h>

enum {
    RTL_CONFIG_1            = 0x52,
    RTL_MSR                 = 0x58,
    RTL_COMMAND_REG         = 0x37,
    RTL_RX_BUF_START        = 0x30,
    RTL_IMR                 = 0x3C,
    RTL_ISR                 = 0x3E,
};

enum isr {
    ISR_SERR = (1 << 15),
    ISR_TIMEOUT = (1 << 14),
    ISR_TER = (1 << 3),
    ISR_TOK = (1 << 2),
    ISR_RER = (1 << 1),
    ISR_ROK = (1 << 0),
};

enum {
    CMD_RST                 = (1 << 4),
};

#define RX_BUF_SIZE 0x2010
static uint32_t *rx_buffer;
static uint16_t io_addr;

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
    uint16_t isr = inw(io_addr + RTL_ISR);

    switch(isr) {
    case ISR_RER:
        break;
    case ISR_ROK:
        break;
    case ISR_TER:
        break;
    case ISR_TOK:
        break;
    }
}

void rtl_init_isr(uint16_t io_addr) {
    outw(io_addr + RTL_IMR, 0xE03F); // enable all interrupts

    add_irq_handler(IRQ9, &rtl_handler);
    add_irq_handler(IRQ10, &rtl_handler);
    add_irq_handler(IRQ11, &rtl_handler);
}

int rtl_master_bus(void) {
    uint32_t command = pci_read_reg(0, 3, 0, 1 << 2);
    pci_write_reg(0, 3, 0, 1 << 2, command | PCI_COM_BUS_MASTER);

    command = pci_read_reg(0, 3, 0, 1 << 2);

    if (command & PCI_COM_BUS_MASTER) {
        return 1;
    }
    
    klog("PCI RTL8139 Command Register set up: %x\n", command);
    return 0;
}

void rtl8139_init(uint8_t bus, uint8_t dev_num, uint16_t io_addr) {
    /* pci_get_io_base(bus, dev_num); */
    klog("RTL IO base addr %x\n", io_addr);
    
    return;
    if (rtl_master_bus()) {
        klog_error("PCI DMA is unavailable\n");
    }

    outb(io_addr + RTL_CONFIG_1, 0x0);
    
    rtl_reset(io_addr);
    rtl_init_isr(io_addr);
}

