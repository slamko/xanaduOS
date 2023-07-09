#include "drivers/int.h"
#include <stdint.h>

void general_handler(struct isr_handler_args *args, const char *msg);

void exception_handlers_init(void);
