#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"
#include "bin/shell.h"
#include "drivers/keyboard.h"

void kernel_main(void) {
    init_gdt();
    init_idt();

    /* fb_print_num(123); */
    receiver_f[0] = &read_stream;
    shell_start();
    
    while(1);
}
