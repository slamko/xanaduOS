#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

#define asm_call(body)                                                         \
    {                                                                          \
        __asm__ volatile("pushal;");                                           \
        body __asm__ volatile("popal;");                                       \
    }

#define foreach(var, action)                                                   \
    for (; var; var = var->next) {                                             \
        action;                                                                \
    }

#define doubly_ll_remove(list, member)                                         \
    if (member->next) {                                                        \
        member->next->prev = member->prev;                                     \
    }                                                                          \
    if (member->prev) {                                                        \
        member->prev->next = member->next;                                     \
    }                                                                          \
    if (*list == member) {                                                     \
        *list = NULL;                                                          \
    }

#define doubly_ll_insert(list, member)                                         \
    void *next = list;                                                         \
    list = member;                                                             \
    member->next = next;                                                       \
    if (member->next) {                                                        \
        member->next->prev = member;                                           \
    }                                                                          \
    member->prev = NULL;

#define single_ll_insert(list, member)                                         \
    void *next = list;                                                         \
    list = member;                                                             \
    member->next = next;

void outb(uint16_t port, uint8_t data);

uint8_t inb(uint16_t port);

void outw(uint16_t port, uint16_t data);

uint16_t inw(uint16_t port);

void outl(uint16_t port, uint32_t data);

uint32_t inl(uint16_t port);

void klog(const char *msg, ...);

void debug_log(const char *msg, ...);

void klog_warn(const char *msg, ...);

void klog_error(const char *msg, ...);

void slab_test(void);

#endif
