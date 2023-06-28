#include "lib/kernel.h"
#include <stdint.h>

enum ata_registers {
    ATA_DATA            = 0x0,
    ATA_ERROR           = 0x1,
    ATA_FEATURES        = 0x1,
    ATA_SECT_CNT        = 0x2,
    ATA_SECT_NUM        = 0x3,
    ATA_CYL_LOW         = 0x4,
    ATA_CYL_HIGH        = 0x5,
    ATA_DRIVE_HEAD      = 0x6,
    ATA_STATUS          = 0x7,
    ATA_COMMAND         = 0x7,
};

enum {
    ATA_ST_BUSY       = (1 << 7),
    ATA_ST_READY      = (1 << 6),
    ATA_ST_WR_FAULT   = (1 << 5),
    ATA_ST_SEEK_CMPL  = (1 << 4),
    ATA_ST_RQ_READY   = (1 << 3),
    ATA_ST_CORRECT    = (1 << 2),
    ATA_ST_ID         = (1 << 1),
    ATA_ST_ERR        = (1 << 0),
};

enum ata_drive_types {
    ATA_PATAPI        = 0xEB14,
    ATA_SATAPI        = 0x9669,
    ATA_PATA          = 0x0000,
    ATA_SATA          = 0xC33C,
};

struct ata_bus {
    uint16_t io_base;
    uint16_t control_base;
};

void ata_detect(struct ata_bus *bus, uint8_t drive) {
    outb(bus->io_base + ATA_DRIVE_HEAD, 0xA | (drive << 4));

    for (unsigned int i = 0; i < 14; i++) {
        inb(bus->control_base);
    }

    int drive_type = -1;
    uint16_t ata = (uint16_t)inb(bus->io_base + ATA_CYL_LOW) & 0xFF;
    ata |= ((uint16_t)inb(bus->io_base + ATA_CYL_HIGH) << 8);

    switch (ata) {
    case ATA_PATAPI:
        klog("Detected PATAPI drive\n");
        break;
    case ATA_SATAPI:
        klog("Detected SATAPI drive\n");
        break;
    case ATA_PATA:
        klog("Detected PATA drive\n");
        break;
    case ATA_SATA:
        klog("Detected SATA drive\n");
        break;
    default:
        klog("Unknown ATA drive detected: %x\n", ata);
    }
}

void ata_init(void) {
    struct ata_bus prim_bus;
    prim_bus.io_base = 0x1F0;
    prim_bus.control_base = 0x3F6;
    
    ata_detect(&prim_bus, 1);

    klog("ATA controller set up\n");
}


