#include "drivers/dev.h"
#include "drivers/fb.h"
#include <stddef.h>

void fprintf(struct char_buffer char_dev, const char *msg) {
    fb_print_black(msg);
    /* for (size_t i = char_dev.pos; i < char_dev.size && msg[i] > 0; i++) { */
        /* char_dev.buf[i] = msg[i]; */
        /* char_dev.pos++; */
    /* } */
}

void flush(struct char_buffer dev) {
    /* dev.flush(); */
}
