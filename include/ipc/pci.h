#ifndef PCI_H
#define PCI_H

#include <stdint.h>

enum command_reg {
    PCI_COM_IO_SPACE = (1 << 0),
    PCI_COM_MEMORY_SPACE = (1 << 1),
    PCI_COM_BUS_MASTER = (1 << 2),
    PCI_COM_MEM_WRITE = (1 << 4),
};

typedef int (*pci_dev_init)(uint8_t, uint8_t, uint8_t, uint16_t);

void pci_register_dev(uint32_t device_id, pci_dev_init dev_init);

void pci_enumeration(void);

void pci_write_reg(uint32_t bus, uint32_t device, uint32_t function,
                   uint32_t reg_offset, uint32_t data);

uint32_t pci_read_reg(uint32_t bus, uint32_t device, uint32_t function,
                      uint32_t reg_offset);

void pci_init(void);

#endif
