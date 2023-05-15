#include <stddef.h>
#include <stdint.h>

void *memset(void *buf, int val, size_t siz);

size_t strlen(const char *str);

int strcmp(const char *str, const char *cmp); 

int strncmp(const char *str, const char *cmp, size_t len); 

size_t strnlen(const char *str, size_t len);
 
void io_wait();    
