#ifndef SLIB_H
#define SLIB_H

#include <stddef.h>
#include <stdint.h>

#define ARR_SIZE(obj) (sizeof(obj) / sizeof(*obj))

#define GET_BIT(obj, bit) (((obj) & (1 << (bit))) >> bit)

void *memset(void *buf, int val, size_t siz);

void *memcpy(void *buf, const void *cpy, size_t len);

char *strcpy(char *str, const char *cpy, size_t len); 

size_t strlen(const char *str);

int strcmp(const char *str, const char *cmp); 

int strneq(const char *str, const char *cmp, size_t len); 

size_t strnlen(const char *str, size_t len);
 
void io_wait();    

#endif
