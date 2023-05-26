#include <stdint.h>
#include "int.h"

struct x86_seg_regs {
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
} __attribute__((packed));

struct tss_entry {
    uint32_t link;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    struct x86_cpu_state cpu;
    struct x86_seg_regs segmets;
    uint32_t ldtr;
    uint32_t iopb;
    uint32_t ssp;
} __attribute__((packed));

extern struct tss_entry tss;

void load_tss(void);

