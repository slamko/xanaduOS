#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "bin/shell.h"
#include "lib/slibc.h"
#include "drivers/keyboard.h"

void shell_prompt() {
    fb_print_attrs("slavos> ", (struct fb_attr) {.non_deletable = 1});
}

void shell_start(void) {
    shell_prompt();
}

int execute(const char *cmd) {
    if (strneq(cmd, "echo", 4)) {
        fb_print_black(cmd + 5);
        return 0;
    } else if (cmd[0] != '\n') {
        fb_print_black("Unknown command");
        fb_newline();
    }
    return 1;
}

int read_stream(unsigned char c) {
    fb_putc(c);

    if (c == '\n') {
        /* fb_print_black(k_buf); */
        /* fb_newline(); */
        execute(kbd_buf);
        shell_prompt();
    }

    return 0;
}

