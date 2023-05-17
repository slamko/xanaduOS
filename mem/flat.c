#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct page {
    uint32_t base;
    uint32_t limit;
    uint8_t used;
    uint8_t map;
};

struct page pdt[128];

#define PDT_SIZE sizeof(pdt) / sizeof(*pdt)

void pdt_init() {
    for (uint32_t i = 0; i < PDT_SIZE; i++) {
        pdt[i] = (struct page) {0};
        pdt[i].base = 0xffff + (i * 0x1000);
        pdt[i].limit = 0x1000;
    }
}

void *malloc(size_t siz) {
    for (uint32_t i = 0; i < PDT_SIZE; i++) {
        if (!pdt[i].used) {
            if (siz <= pdt[i].limit) {
                pdt[i].used = true;
                return (void *)pdt[i].base;
            } else {
                pdt[i].map = 1;
            }
        }
    }

    return NULL;
}

void free(void *ptr) {
    if (!ptr) return;

    for (uint32_t i = 0; i < PDT_SIZE; i++) {
        if (pdt[i].base <= (uint32_t)ptr &&
            (pdt[i].base + pdt[i].limit) > (uint32_t)ptr) {
            pdt[i].used = false;
            return;
        }
    }
}

void init() {
    pdt_init();
}

    
