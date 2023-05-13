#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/gdt.h"
#include "drivers/int.h"

uint16_t row = 0;

void prompt() {
    fb_print_black("slavos>");
    fb_newline();
}

void shell(void) {
    fb_clear();

    for (int i = 0; i < 8; i++) {
        prompt();
        row ++; 
    }
}

void kernel_main(void) {
    init_gdt();
    init_idt();
    
    shell();
    
    while(1);
}
