#define KBD_INPUT_PORT 0x60
#define KBD_STATUS_PORT 0x64

void interrupt();

typedef int (*receiver)(unsigned char);

extern receiver receiver_f;
