#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "bin/shell.h"
#include "lib/slibc.h"
#include "drivers/keyboard.h"
#include "mem/flat.h"

static char last_cmd[1024];
static char cur_cmd[1024];

void shell_prompt() {
    fb_print_attrs("slavos> ", (struct fb_attr) {.non_deletable = 1});
}

void shell_start(void) {
    fb_clear();
    receiver_f[0] = &read_stream;
    shell_prompt();
}

int execute(const char *cmd, size_t len) {
    if (strneq(cmd, "echo", 4) == 0) {
        fb_print_black(cmd + 5);
        fb_newline();
        return 0;
    } else if (cmd[0] || len == 0) {
        fb_print_black("Unknown command");
        fb_newline();
    }
    return 1;
}

int read_stream(uint32_t c) {
    char *cmd = NULL;
    size_t len;

    switch (c) {
    case UP_ARROW:

        fb_last_written_buf(&cmd, &len);
        memcpy(cur_cmd, cmd, len);
        
        fb_delete_last(len);
        fb_print_black(last_cmd);
        break;
    case DOWN_ARROW:
        fb_last_written_buf(&cmd, &len);

        fb_delete_last(len);
        fb_print_black(cur_cmd);
        break;
    default:
        fb_putc((uint8_t)c);
        break;
    }
    

    if (c == '\n') {
        /* fb_print_black(k_buf); */
        /* fb_newline(); */

        fb_last_written_buf(&cmd, &len);
        memcpy(last_cmd, cmd, len);

        execute(cmd, len);
        shell_prompt();
    }

    return 0;
}

