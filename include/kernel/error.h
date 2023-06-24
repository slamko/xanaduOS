#ifndef ERROR_H
#define ERROR_H

enum ErrorType {
    EIO         = 0x1, 
    EINVAL      = 0x2, 
    ENOMEM      = 0x3, 
};

struct error_state {
    const char *msg;
    enum ErrorType err;
};

int panic(const char *msg, struct error_state);

#endif
