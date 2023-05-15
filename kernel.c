#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "drivers/shell.h"
#include "drivers/keyboard.h"

void kernel_main(void) {
    init_gdt();
    init_idt();
    fb_clear();

    receiver_f = &read_stream;
    shell_start();
    
    while(1);
}
