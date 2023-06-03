enum {
    PS2_COMMAND_PORT = 0x64,
    PS2_DATA_PORT = 0x60
};

enum {
    PS2_ENABLE_AUX = 0xA8,
    PS2_ACKNOWLEDGE = 0xFA,
    PS2_GET_COMPAQ_STAT = 0x20,
    PS2_SET_COMPAQ_STAT = 0x60,
    PS2_SET_DEFAULTS = 0xF6,
    PS2_ENABLE_STREAMING = 0xF4,
};

int ps2_init(void);
