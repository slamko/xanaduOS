#define KBD_INPUT_PORT 0x60
#define KBD_STATUS_PORT 0x64

#define KBD_INT_REC_NUM 256
#define KBD_DEL 127

void interrupt();

typedef int (*receiver)(unsigned char);

extern receiver receiver_f[KBD_INT_REC_NUM];
extern char kbd_buf[1024];
