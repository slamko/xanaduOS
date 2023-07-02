#include "drivers/int.h"
#include "drivers/pit.h"
#include "lib/kernel.h"
#include "lib/slibc.h"
#include "drivers/cmos.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define FLOPPY_IRQ 6
#define TIMEOUT_MS 30000

enum floppy_registers
{
   FLOPPY_STATUS_A                         = 0x0, // read-only
   FLOPPY_STATUS_B                         = 0x1, // read-only
   FLOPPY_DIGITAL_OUT_REG                  = 0x2,
   FLOPPY_TAPE_DRIVE_REG                   = 0x3,
   FLOPPY_MAIN_STATUS_REG                  = 0x4, // read-only
   FLOPPY_DATARATE_SELECT_REG              = 0x4, // write-only
   FLOPPY_DATA_FIFO                        = 0x5,
   FLOPPY_DIGITAL_IN_REG                   = 0x7, // read-only
   FLOPPY_CONFIG_REG                       = 0x7  // write-only
};

enum rotate_speed {
    FLOPPY_500KBPS  = 0,
    FLOPPY_300KBPS  = 1,
    FLOPPY_250KBPS  = 2,
    FLOPPY_1MBPS    = 3,
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

enum msr {
    MSR_RQM             = (1 << 7),
    MSR_DIO             = (1 << 6),
    MSR_NDMA            = (1 << 5),
    MSR_CD              = (1 << 4),
    MSR_ACTD            = (1 << 3),
    MSR_ACTC            = (1 << 2),
    MSR_ACTB            = (1 << 1),
    MSR_ACTA            = (1 << 0),
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

enum floppy_error {
    FLOPPY_TIMEOUT = -1,
    FLOPPY_FAULT = -2,
};

static uint8_t floppy_drive = 0;
static uint16_t floppy_base = 0x3F0;
static volatile tick_t floppy_irq_tick;

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

enum floppy_drive_type floppy_type[4];

int16_t floppy_wait(uint16_t base) {
    tick_t ticks = 0;
    uint32_t delay = 10;
    uint8_t msr = inb(base + FLOPPY_MAIN_STATUS_REG);

    while(!(msr & MSR_RQM) || (msr & MSR_DIO)) {
        msr = inb(base + FLOPPY_MAIN_STATUS_REG);
        ticks += delay;

        if (ticks >= TIMEOUT_MS) {
            return 1;
        }
        sleep_ms(delay);
    }
    return 0;
}

int16_t floppy_read(uint16_t base) {
    if (floppy_wait(base)) {
        return FLOPPY_TIMEOUT;
    }
   
    return inb(base + FLOPPY_DATA_FIFO);
}

int floppy_write_cmd(uint16_t base, uint8_t cmd, size_t param_cnt, ...) {
    uint8_t msr = inb(base + FLOPPY_MAIN_STATUS_REG);

    if (!(msr & MSR_RQM) || msr & MSR_DIO) {
        return FLOPPY_FAULT;
    }

    outb(base + FLOPPY_DATA_FIFO, cmd);

    va_list args;
    va_start(args, param_cnt);

    for (unsigned int i = 0; i < param_cnt; i++) {
        if (floppy_wait(base)) {
            return FLOPPY_TIMEOUT;
        }
        
        outb(base + FLOPPY_DATA_FIFO, va_arg(args, int));
    }

    va_end(args);
    return 0;
}

int floppy_irq_wait(tick_t init_tick) {
    tick_t timeout_tick = 0;
    while(floppy_irq_tick == init_tick) {
        sleep_ms(10);
        timeout_tick += 10;

        if (timeout_tick > TIMEOUT_MS) {
            return FLOPPY_TIMEOUT;
        }
    }

    return 0;
}

int floppy_sense_interrupt(uint16_t base) {
    int ret;
    if ((ret = floppy_write_cmd(base, CMD_SENSE_INTERRUPT, 0))) {
        return ret;
    }

    int val = floppy_read(base);
    if (val < 0) {
        return val;
    }
    
    int lh = floppy_read(base);

    if (lh < 0) {
        return lh;
    }

    val |= (lh << 8);

    klog("Interrupt sensed");
    return val;
}

void floppy_motor(uint16_t base, uint8_t drive, uint8_t motor_on) {
    uint8_t motor = ((1 << drive) & (motor_on << drive)) << 4;
    outb(base + FLOPPY_DIGITAL_OUT_REG,
         DOR_IRQ | DOR_RESET| (drive & 0x3) | motor);
    sleep_ms(1000);
}

int floppy_calibrate(uint16_t base) {
    floppy_motor(base, floppy_drive, 1);

    for (unsigned int i = 0; i < 3; i++) {
        klog("Floppy calibration\n");
        tick_t irq_tick = floppy_irq_tick;

        if (floppy_write_cmd(base, CMD_RECALIBRATE, 1, floppy_drive)) {
            return -1;
        }

        if (floppy_irq_wait(irq_tick)) {
            klog_warn("Floppy IRQ wait timeout\n");
            continue;
        }

        int sense = floppy_sense_interrupt(base);
        if (sense == -1) {
            klog_warn("Sense interupt failed\n");
            continue;
        }

        uint16_t usense = (uint16_t)sense;
        
        uint8_t st1 = usense >> 8;
        uint8_t cyl = usense & 0xFF;

        if ((st1 & (0xC0 | floppy_drive)) != (0xC0 | floppy_drive)) {
            klog_warn("Sense interupt failed\n");
            continue;
        }

        if (!cyl) {
            floppy_motor(base, floppy_drive, 0);
        }
    }

    klog_error("Floppy calibration failed\n");
    floppy_motor(base, floppy_drive, 0);
    return 1;
}

int floppy_reset(uint16_t base) {
    uint8_t dor = inb(base + FLOPPY_DIGITAL_OUT_REG);
    sleep_us(100);

    outb(base + FLOPPY_DIGITAL_OUT_REG, 0);
    sleep_ms(5);
    /* outb(FLOPPY_DIGITAL_OUT_REG, dor | (1 << 2)); */
    outb(base + FLOPPY_DIGITAL_OUT_REG, DOR_IRQ | DOR_RESET);

    if (floppy_sense_interrupt(base) == -1) {
        return -1;
    }

    uint8_t speed = FLOPPY_500KBPS;

    if (floppy_type[floppy_drive] == FLOPPY_2_88MB_3_5) {
        speed = FLOPPY_1MBPS;
    }
    
    if (floppy_write_cmd(base + FLOPPY_CONFIG_REG, speed, 0) == -1) {
        return -1;
    }

    if (floppy_write_cmd(base + CMD_SPECIFY, 2, 0xDF, 0x02) == -1) {
        return -1;
    }

    if (floppy_calibrate(base)) {
        return -1;
    }
         
    return 0;
}

void floppy_detect(void) {
    cmos_select_reg(0x10);
   
    uint8_t dtypes = cmos_read_data();
    if (dtypes >> 4) {
        klog("Detected Master floppy drive: %s\n", drive_types[dtypes >> 4]);
        floppy_type[floppy_drive] = dtypes >> 4;
    }

    if (dtypes & 0x0F) {
        klog("Detected Slave floppy drive: %s\n", drive_types[dtypes & 0x0F]);
        floppy_type[floppy_drive] = dtypes & 0x0F;
    }

    floppy_base = 0x3F0;
}

void floppy_handler(struct isr_handler_args args) {
    klog("Floppy\n");
    floppy_irq_tick++;
}

void floppy_init(void) {
    floppy_detect();
    add_irq_handler(FLOPPY_IRQ, &floppy_handler);

    if (floppy_reset(floppy_base) == -1) {
        klog_error("error: Could not configure floppy disk\n");
    }
    
    klog("Floppy controller initialized\n");
}
