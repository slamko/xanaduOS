#include <stdint.h>
#include "drivers/fb.h"
#include "drivers/shell.h"
#include "lib/slibc.h"

void shell_prompt() {
    /* fb_newline(); */
    fb_print_black("slavos> ");
}

void shell_start(void) {
    shell_prompt();
}

int read_stream(unsigned char c) {
    fb_putc(c);

    if (c == '\n') {
        shell_prompt();
    }

    return 0;
}

