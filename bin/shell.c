#include <stddef.h>
#include <stdint.h>
#include "drivers/fb.h"
#include "bin/shell.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
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
    kbd_add_receiver(&read_stream);
    shell_prompt();
}

void serial_shell(void) {
    char buf[256];

    while (1) {
        serial_read_buf(COM1, buf, ARR_SIZE(buf));
        serial_print(COM1, buf);

        serial_print(COM1, "\r\nslavos> ");
        memset(buf, 0, ARR_SIZE(buf));
    }
}

int execute(const char *cmd, size_t len) {
    sleep_ms(900);
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
        /* fb_print_black("hello"); */
        /* fb_newline(); */

        /* fb_last_written_buf(&cmd, &len); */


        /* execute(cmd, len); */
        shell_prompt();
    }

    /* asm volatile("int $0x6"); */

    return 0;
}

