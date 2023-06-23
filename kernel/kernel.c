#include "lib/kernel.h"
#include "bin/shell.h"
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "io.h"
#include "kernel/syscall.h"
#include "lib/slibc.h"
#include "mem/allocator.h"
#include "mem/flat.h"
#include "mem/paging.h"
#include <stdint.h>

void jump_usermode(void);

void kernel_main(void) {
    fb_clear();

    init_gdt();
    init_idt();
    paging_init();
    /* klog("Paging enabled!"); */
    /* alloc_test(); */

    serial_init();

    kbd_init();
    ps2_init();
    pit_init(0);
    syscall_init();

    klog("Hello paging!\n");
    /* jump_usermode(); */

    while (1)
        ;
}
