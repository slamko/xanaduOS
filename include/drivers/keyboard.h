#include <stdint.h>
#define KBD_INPUT_PORT 0x60
#define KBD_STATUS_PORT 0x64

#define KBD_INT_REC_NUM 256
#define KBD_DEL 127

enum SPEC_CODES {
    UP_ARROW = 344,
    DOWN_ARROW = 345
};

void interrupt();

typedef int (*receiver)(uint32_t);

extern receiver receiver_f[KBD_INT_REC_NUM];
extern uint32_t kbd_buf[1024];
