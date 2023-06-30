#include <stdint.h>
#include "lib/kernel.h"

enum {
    CONFIG_ADDRESS          = 0xCF8,
    CONFIG_DATA             = 0xCFC,
    COMMAND_REG_OFFSET      = 0x4,
};

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

uint16_t get_device_id(uint8_t bus, uint16_t device) {
    return 0;
}

void lookup_pci_dev(void) {
 for (unsigned int bus = 0; bus < 16; bus++) {
        for (unsigned int dev = 0; dev < 16; dev++) {

            uint32_t val = pci_read_reg(bus, dev, 0, 0);
            uint16_t vendor_id = val & 0xFFFF;
            uint16_t device_id = val >> 16;
            
            if (vendor_id != 0xFFFF) {
                klog("Vendor ID bus %u, dev %u: %x, %x\n",
                     bus, dev, device_id, vendor_id);
            }
        }
    }
}
