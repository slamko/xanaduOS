#include "drivers/int.h"
#include "drivers/pit.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "drivers/cmos.h"
#include <stdint.h>

// standard base address of the primary floppy controller
static const int floppy_base = 0x03f0;
// standard IRQ number for floppy controllers
#define FLOPPY_IRQ 6

enum floppy_registers
{
   FLOPPY_STATUS_A                         = 0x3F0, // read-only
   FLOPPY_STATUS_B                         = 0x3F1, // read-only
   FLOPPY_DIGITAL_OUT_REG                  = 0x3F2,
   FLOPPY_TAPE_DRIVE_REG                   = 0x3F3,
   FLOPPY_MAIN_STATUS_REG                  = 0x3F4, // read-only
   FLOPPY_DATARATE_SELECT_REG              = 0x3F4, // write-only
   FLOPPY_DATA_FIFO                        = 0x3F5,
   FLOPPY_DIGITAL_IN_REG                   = 0x3F7, // read-only
   FLOPPY_CONFIG_REG                       = 0x3F7  // write-only
};

enum floppy_commands {
   CMD_SPECIFY = 3,            // SPECIFY
   CMD_WRITE_DATA = 5,         // WRITE DATA
   CMD_READ_DATA = 6,          // READ DATA
   CMD_RECALIBRATE = 7,        // RECALIBRATE
   CMD_SENSE_INTERRUPT = 8,    // SENSE INTERRUPT
   CMD_SEEK = 15,              // SEEK
};

enum floppy_drive_type {
    FLOPPY_NO_DRIVE     = 0,
    FLOPPY_360KB_5_25   = 1,
    FLOPPY_1_2MB_5_25   = 2,
    FLOPPY_720KB_3_5    = 3,
    FLOPPY_1_44MB_3_5   = 4,
    FLOPPY_2_88MB_3_5   = 5,
};

enum dor {
    DOR_MOTD            = (1 << 7),
    DOR_MOTC            = (1 << 6),
    DOR_MOTB            = (1 << 5),
    DOR_MOTA            = (1 << 4),
    DOR_IRQ             = (1 << 3),
    DOR_RESET           = (1 << 2),
    DOR_DSEL            = 0x3,
};

static const char *drive_types[8] = {
    "none",
    "360kB 5.25\"",
    "1.2MB 5.25\"",
    "720kB 3.5\"",

    "1.44MB 3.5\"",
    "2.88MB 3.5\"",
    "unknown type",
    "unknown type"
};

enum cmos {
    CMOS_COMMAND_PORT = 0x70,
    CMOS_DATA_PORT    = 0x71,
};

enum floppy_drive_type floppy_type;

void floppy_detect(void) {
    cmos_select_reg(0x10);
   
    uint8_t dtypes = cmos_read_data();
    if (dtypes >> 4) {
        klog("\nDetected Master floppy drive: ");
        klog(drive_types[dtypes >> 4]);
    }

    if (dtypes & 0x0F) {
        klog("\nDetected Slave floppy drive: ");
        klog(drive_types[dtypes & 0x0F]);
    }
    klog("\n");
}

void run_motor(uint8_t drive) {
    uint8_t motor = (1 << drive) << 4;
    outb(FLOPPY_DIGITAL_OUT_REG, (drive & 0x3) | motor);
    sleep_ms(1000);
}

void floppy_handler(struct isr_handler_args args) {
    klog("Floppy");
}

void floppy_init(void) {
    floppy_detect();
    
    add_isr_handler(FLOPPY_IRQ, &floppy_handler, 0);
    klog("Floppy controller initialized");
}
