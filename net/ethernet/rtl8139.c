#include "net/ethernet/rtl8139.h"
#include "drivers/int.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "mem/allocator.h"
#include "ipc/pci.h"
#include "mem/paging.h"
#include <stddef.h>
#include <stdint.h>

enum {
    RTL_CONFIG_1            = 0x52,
    RTL_MSR                 = 0x58,
    RTL_COMMAND_REG         = 0x37,
    RTL_RX_BUF_START        = 0x30,
    RTL_IMR                 = 0x3C,
    RTL_ISR                 = 0x3E,
    RTL_RCR                 = 0x44,
    RTL_TX_STAT             = 0x10,
    RTL_TX_START            = 0x20,
};

enum isr {
    ISR_SERR                = (1 << 15),
    ISR_TIMEOUT             = (1 << 14),
    ISR_TER                 = (1 << 3),
    ISR_TOK                 = (1 << 2),
    ISR_RER                 = (1 << 1),
    ISR_ROK                 = (1 << 0),
};

enum rcr {
    RCR_WRAP                = (1 << 7),
    RCR_ACC_ERR             = (1 << 5),
    RCR_ACC_RUNT            = (1 << 4),
    RCR_ACC_BROAD           = (1 << 3),
    RCR_ACC_MULTIC          = (1 << 2),
    RCR_ACC_PHYS_MATCH      = (1 << 1),
    RCR_ACC_ALL             = (1 << 0),
};

enum tx {
    TX_OWN = (1 << 13),
};

enum cmd {
    CMD_RX_EN               = (1 << 3),
    CMD_TX_EN               = (1 << 2),
};

enum {
    CMD_RST                 = (1 << 4),
};

#define WRAP_PACKETS 1

#if WRAP_PACKETS
#define RX_BUF_SIZE (0x4010 + 1500)
#else
#define RX_BUF_SIZE 0x4010
#endif

static uint32_t *rx_buffer;
struct pci_dev_data *dev;
static uint8_t tx_d;

void rtl_reset(uint16_t io_addr) {
    outb(io_addr + RTL_COMMAND_REG, CMD_RST);
    while(inb(io_addr + RTL_COMMAND_REG) & CMD_RST);
}

void rtl_init_rx_buf(uint16_t io_addr) {
    uintptr_t buf_phys_addr;
    rx_buffer = kmalloc_phys(RX_BUF_SIZE, &buf_phys_addr);

    outl(io_addr + RTL_RX_BUF_START, buf_phys_addr);
}

void rtl_rx_tx_config(uint16_t io_addr) {
    outl(io_addr + RTL_RCR, (1 << 11)
         #if WRAP_PACKETS
         | RCR_WRAP
         #endif
         | RCR_ACC_ERR
         | RCR_ACC_RUNT
         | RCR_ACC_BROAD
         | RCR_ACC_MULTIC
         | RCR_ACC_PHYS_MATCH
         | RCR_ACC_ALL);

    outl(io_addr + RTL_COMMAND_REG, CMD_RX_EN | CMD_TX_EN);
}

void rtl_handler(struct isr_handler_args *args) {
    klog("RTL isr\n");
    uint16_t isr = inw(dev->io_addr + RTL_ISR);

    if (isr & ISR_ROK) {
        outw(dev->io_addr + RTL_ISR, ISR_ROK);

    }

    if (isr & ISR_TOK) {
        outw(dev->io_addr + RTL_ISR, ISR_TOK);

    }
    
}

void rtl_send(uint32_t *buf, size_t size) {
    uintptr_t phys_addr = ptr_to_phys_addr(cur_pd, buf);
    uint16_t stat_port = dev->io_addr + RTL_TX_STAT + (tx_d * 0x4);

    while(!(inl(stat_port) & TX_OWN));

    outl(dev->io_addr + RTL_TX_START + (tx_d * 0x4), phys_addr);
    outl(stat_port, (size & 0x1FFF));
    klog("Packet sent %x\n", inl(stat_port)); 
    
    tx_d++;
}

void rtl_init_isr(uint16_t io_addr, uint8_t irq) {
    outw(io_addr + RTL_IMR, 0xF); // enable all interrupts

    add_irq_handler(IRQ9, &rtl_handler);
    add_irq_handler(IRQ10, &rtl_handler);
    add_irq_handler(IRQ11, &rtl_handler);

    klog("RTL8139 IRQ %x\n", inw(io_addr + RTL_IMR));
}

int rtl_master_bus(void) {
    uint32_t command = pci_read_reg(dev->bus, dev->dev, 0, PCI_REG_COMMAND);
    if (command & PCI_COM_BUS_MASTER) {
        return 0;
    }
    
    pci_write_reg(dev->bus, dev->dev, 0, PCI_REG_COMMAND,
                  command | PCI_COM_BUS_MASTER);

    command = pci_read_reg(dev->bus, dev->dev, 0, PCI_REG_COMMAND);

    if (!(command & PCI_COM_BUS_MASTER)) {
        return 1;
    }
    
    klog("PCI RTL8139 Command Register set up: %x\n", command);
    return 0;
}

int rtl8139_init(struct pci_dev_data *dev_data) {
    /* pci_get_io_base(bus, dev_num); */
    klog("RTL IO base addr %x\n", dev_data->io_addr);
    dev = dev_data;
    
    if (rtl_master_bus()) {
        klog_error("PCI DMA is unavailable\n");
    }

    outb(dev_data->io_addr + RTL_CONFIG_1, 0x0);
    rtl_reset(dev_data->io_addr);
    rtl_init_rx_buf(dev_data->io_addr);
    rtl_init_isr(dev_data->io_addr, dev_data->irq);
    rtl_rx_tx_config(dev_data->io_addr);

    klog("RTL8139 NIC configured\n");
/*
    uint8_t data[92];
    for (unsigned int i = 0; i < 7; i++) {
        data[i] = 0xAA;
    }

    data[7] = 0xAB;
    for (unsigned int i = 8; i < 14; i++) {
        data[i] = 0xFF;
    }

    for (unsigned int i = 14; i < 20; i++) {
        data[i] = 0xA5;
    }
    data[20] = 0;
    data[21] = 64;
*/

    uint8_t data[92];
    uint32_t len = ARR_SIZE(data);
    for (unsigned int i = len; i < len - 7; i++) {
        data[i] = 0xAA;
    }

    data[7] = 0xAB;
    for (unsigned int i = len - 8; i < len - 14; i++) {
        data[i] = 0xFF;
    }

    for (unsigned int i = len - 14; i < len - 20; i++) {
        data[i] = 0xA5;
    }
    data[len - 20] = 0;
    data[len - 21] = 64;
    
    rtl_send((uint32_t *)data, ARR_SIZE(data));
    
    return 0;
}

