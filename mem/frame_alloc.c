#include "drivers/fb.h"
#include "mem/allocator.h"
#include "mem/frame_allocator.h"
#include "lib/kernel.h"
#include "mem/paging.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

uint32_t *frames;
uint32_t frames_num;
uint32_t mem_limit;

bool is_free_frame(uint32_t frame_map, uint32_t frame) {
    return !(frames[frame_map] & (1 << frame));
}

void get_frame_meta(uintptr_t addr,
                    unsigned int *frame_map, unsigned int *frame) {

    *frame_map = addr / (0x1000 * sizeof(*frames));
    *frame = (addr - (*frame_map * 0x1000 * sizeof(*frames))) / 0x1000;
}


uintptr_t alloc_frame(uintptr_t addr, unsigned int flags) {
    if (addr) {
        unsigned int frame_map, frame;
        get_frame_meta(addr, &frame_map, &frame);
        frames[frame_map] |= (1 << frame);

        return addr | flags;
    }
    
    for (unsigned int i = 0; i < frames_num; i++) {
        if (frames[i] & 0xFFFFFFFF) continue;
        
        for (unsigned int j = 0; j < sizeof(*frames); j++) {
            if (is_free_frame(i, j)) {
                frames[i] |= (1 << j);
                return ((i * 0x1000 * sizeof(*frames)) + (j * 0x1000)) | flags;
            }
        }
    }

    return 0;
}

uintptr_t find_alloc_frame(unsigned int flags) {
    return alloc_frame(0, flags);
}

uintptr_t alloc_pt(unsigned int pde, unsigned int pte, uint16_t flags) {
    uintptr_t phys_addr = 0;
    uintptr_t *new_pt = kmalloc_align_phys(PAGE_SIZE, PAGE_SIZE, &phys_addr);
    memset(new_pt, 0, PAGE_SIZE);
    /* klog("kmalloc"); */

    new_pt[pte] = find_alloc_frame(flags);
    klog("pte: ");
    fb_print_hex((uintptr_t)new_pt[pte]);
    fb_print_hex((uintptr_t)phys_addr);
    return phys_addr | flags;
}

int dealloc_frame(uintptr_t addr) {
    if (!addr) {
        return 1;
    }

    addr &= ~0xfff;

    unsigned int frame_map, frame;
    get_frame_meta(addr, &frame_map, &frame);
    frames[frame_map] &= ~(1 << frame);

    return 0;
}

void frame_alloc_init(void) {
    mem_limit = 128 * 1024 * 1024;
    frames_num = mem_limit / 0x1000;
    frames = kmalloc(frames_num / sizeof(*frames));
}
