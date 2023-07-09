#ifndef ERROR_H
#define ERROR_H

enum ErrorType {
    EIO         = 0x1, 
    EINVAL      = 0x2, 
    ENOMEM      = 0x3, 
    ENOENT      = 0x4,
};

struct error_state {
    const char *msg;
    enum ErrorType err;
};

typedef enum ErrorType err_t;

int panic(const char *msg, err_t type);

void error(const char *msg, err_t type);

#endif
