#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/shell.h"

void kernel_main(void) {
    init_gdt();
    init_idt();
    fb_clear();

    shell();
    
    while(1);
}
