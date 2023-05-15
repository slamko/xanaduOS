#define KBD_INPUT_PORT 0x60
#define KBD_STATUS_PORT 0x64

#define KBD_INT_REC_NUM 256

void interrupt();

typedef int (*receiver)(unsigned char);

extern receiver receiver_f[KBD_INT_REC_NUM];
