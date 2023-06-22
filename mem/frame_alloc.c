#include "mem/allocator.h"
#include <stdint.h>
#include <stdbool.h>

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
    alloc_frame(0, flags);
    
    return 0;
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
