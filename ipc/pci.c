#include "ipc/pci.h"
#include <stdint.h>
#include "lib/kernel.h"
#include "net/ethernet/rtl8139.h"

enum {
    CONFIG_ADDRESS          = 0xCF8,
    CONFIG_DATA             = 0xCFC,
    COMMAND_REG_OFFSET      = 0x4,
};

void pci_write_reg(uint32_t bus, uint32_t device, uint32_t function,
                   uint32_t reg_offset, uint32_t data) {
    uint32_t address = (1 << 31)
        | reg_offset
        | ((bus & 0xF) << 16)
        | ((device & 0xF) << 11)
        | ((function & 0x3) << 8);

    outl(CONFIG_ADDRESS, address);
    outl(CONFIG_DATA, data);
}

uint32_t pci_read_reg(uint32_t bus, uint32_t device, uint32_t function,
                      uint32_t reg_offset) {
    uint32_t address = (1 << 31)
        | reg_offset
        | ((bus & 0xF) << 16)
        | ((device & 0xF) << 11)
        | ((function & 0x3) << 8);

    outl(CONFIG_ADDRESS, address);
    uint32_t val = inl(CONFIG_DATA);

    return val;
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t device_num) {
    uint32_t reg = pci_read_reg(bus, device_num, 0, 0);

    return (reg >> 16);
}

uint32_t pci_get_io_base(uint8_t bus, uint8_t device_num) {
    uint8_t header_type = (pci_read_reg(bus, device_num, 0, 0xC) >> 16) & 0xFF;
    uint32_t io_base = 0;

    klog("Header type %x\n", header_type); 

    switch (header_type) {
    case 0x0:;
    case 0x1:;
        io_base = pci_read_reg(bus, device_num, 0, 0x10);
        break;
    case 0x2:;
        io_base = pci_read_reg(bus, device_num, 0, 0x2C);
        return io_base;
    }

    return io_base & ~0x3;
}

void pci_enumeration(void) {
    for (unsigned int bus = 0; bus < 16; bus++) {
        for (unsigned int dev = 0; dev < 16; dev++) {

            uint32_t val = pci_read_reg(bus, dev, 0, 0);
            uint16_t vendor_id = val & 0xFFFF;
            uint16_t device_id = val >> 16;
            
            if (vendor_id != 0xFFFF) {
                klog("Vendor ID bus %u, dev %u: %x, %x\n",
                     bus, dev, device_id, vendor_id);
            }

            if (device_id == 0x8139) {
                uint32_t io_base = pci_get_io_base(bus, dev);
                rtl8139_init(bus, dev, io_base);
            }
        }
    }
}
