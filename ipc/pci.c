#include "ipc/pci.h"
#include "drivers/pic.h"
#include "lib/kernel.h"
#include "net/ethernet/rtl8139.h"
#include "mem/slab_allocator.h"

#include <stdint.h>
#include <string.h>

enum {
    CONFIG_ADDRESS          = 0xCF8,
    CONFIG_DATA             = 0xCFC,
    COMMAND_REG_OFFSET      = 0x4,
};

enum pci_header {
    PCI_END_POINT           = 0x0,
    PCI_BRIDGE              = 0x1,
    PCI_CARDBUS_BRIDGE      = 0x2,
};

typedef enum pci_header header_t;

struct pci_dev {
    struct pci_dev *next;
    pci_dev_init dev_init;
    uint32_t device_id;

    struct pci_dev_data data;
};


union bridge_bus_data {
    struct {
        uint8_t primary_bus;
        uint8_t secondary_bus;
        uint8_t sub_bus_num;
        uint8_t timer;
    };
    uint32_t data;
} __attribute__((packed));

void print_bus_data(union bridge_bus_data bd) {
    klog("Prime bus: %u\n", bd.primary_bus);
    klog("Secondary bus: %u\n", bd.secondary_bus);
    klog("Subordinary bus num: %u\n", bd.sub_bus_num);
}

struct pci_dev *pci_devices;
struct slab_cache *pci_slab;

void pci_register_dev(uint32_t device_id, pci_dev_init dev_init) {
    struct pci_dev *next = pci_devices;

    pci_devices = slab_alloc_from_cache(pci_slab);
    pci_devices->dev_init = dev_init;
    pci_devices->device_id = device_id;
    pci_devices->next = next;
}

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

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device_num) {
    uint32_t reg = pci_read_reg(bus, device_num, 0, 0);

    return (reg & 0xFFFF);
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t device_num) {
    uint32_t reg = pci_read_reg(bus, device_num, 0, 0);

    return (reg >> 16);
}

uint8_t pci_get_header_type(uint8_t bus, uint8_t dev, header_t *header) {
    *header = (pci_read_reg(bus, dev, 0, 0xC) >> 16) & 0xFF;
    return ((1 << 7) & *header);
}

uint32_t pci_get_io_base(uint8_t bus, uint8_t device_num, header_t header) {
    uint32_t io_base = 0;

    switch (header) {
    case 0x0:;
    case 0x1:;
        io_base = pci_read_reg(bus, device_num, 0, 0x10);
        break;
    case 0x2:;
        io_base = pci_read_reg(bus, device_num, 0, 0x2C);
        break;
    }

    return io_base & ~0x3;
}

uint32_t pci_get_irq(uint8_t bus, uint8_t device_num, header_t header_type) {
    uint8_t irq = 0;

    switch (header_type) {
    case 0x0:;
    case 0x1:;
    case 0x2:;
        irq = pci_read_reg(bus, device_num, 0, 0x3C) & 0xFF;
    }

    return irq;
}

void pci_enumeration(void) {
    for (unsigned int bus = 0; bus < 16; bus++) {
        for (unsigned int dev = 0; dev < 16; dev++) {
            uint32_t val = pci_read_reg(bus, dev, 0, 0);
            uint16_t vendor_id = val & 0xFFFF;
            uint16_t device_id = val >> 16;
            header_t header;
           
            if (vendor_id != 0xFFFF) {
                if (pci_get_header_type(bus, dev, &header)) {
                }
                klog("Subclass: %x\n", pci_read_reg(bus, dev, 0, 0x8) >> 16);
                klog("Subclass: %x\n", pci_read_reg(bus, dev, 0, 0x18));

                if (header) {
                    /* union bridge_bus_data bd; */
                    /* bd.data = pci_read_reg(bus, dev, 0, 0x18); */
               }

                klog("Vendor ID bus %u, dev %u: %x, %x\n",
                     bus, dev, device_id, vendor_id);
                klog("Header: %d\n", header);
            }

            struct pci_dev *device = pci_devices;
            for (; device; device = device->next) {
                if (device_id == device->device_id) {
                    uint32_t io_base = pci_get_io_base(bus, dev, header);
                    uint8_t irq = pci_get_irq(bus, dev, header);

                    device->data.bus = bus;
                    device->data.dev = dev;
                    device->data.io_addr = io_base;
                    device->data.irq = irq;

                    device->dev_init(&device->data);
                }
            }
       }
    }
}

uint8_t pci_enumerate_bus(uint8_t bus) {
    uint8_t max_bus = bus;
    
    for (unsigned int dev = 0; dev < 32; dev++) {
        uint16_t vendor_id = pci_get_vendor_id(bus, dev);
        if (vendor_id == 0xFFFF) {
            break;
        }
        
        header_t header;
        pci_get_header_type(bus, dev, &header);

        if (header) {
            union bridge_bus_data bus_info;
            bus_info.data = pci_read_reg(bus, dev, 0, 0x18);

            max_bus++;
            bus_info.primary_bus = bus;
            bus_info.secondary_bus = max_bus;
            bus_info.sub_bus_num = 0xFF;
            pci_write_reg(bus, dev, 0, 0x18, bus_info.data);

            bus_info.sub_bus_num = pci_enumerate_bus(max_bus);
            pci_write_reg(bus, dev, 0, 0x18, bus_info.data);
        }
    }

    return max_bus;
}

void pci_init(void) {
    pci_slab = slab_cache_create(sizeof(*pci_devices));

    pci_register_dev(0x8139, &rtl8139_init);
}
