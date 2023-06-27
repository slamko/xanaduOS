#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

// standard base address of the primary floppy controller
static const int floppy_base = 0x03f0;
// standard IRQ number for floppy controllers
#define FLOPPY_IRQ 6

enum floppy_registers {
   FLOPPY_DOR  = 2,  // digital output register
   FLOPPY_MSR  = 4,  // master status register, read only
   FLOPPY_FIFO = 5,  // data FIFO, in DMA operation for commands
   FLOPPY_CCR  = 7   // configuration control register, write only
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

enum cmos {
    CMOS_COMMAND_PORT = 0x70,
    CMOS_DATA_PORT    = 0x71,
};

enum floppy_drive_type floppy_type;

void floppy_detect(void) {
    outb(CMOS_COMMAND_PORT, 0x10);
    io_wait();
   
    floppy_type = inb(CMOS_DATA_PORT);
    
}

void floppy_handler(struct isr_handler_args args) {
    klog("Floppy");
}

void floppy_init(void) {
    add_isr_handler(FLOPPY_IRQ, &floppy_handler, 0);
    klog("Floppy controller initialized");
}
