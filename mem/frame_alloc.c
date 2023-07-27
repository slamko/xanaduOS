#include "lib/slibc.h"
#include "mem/frame_allocator.h"
#include "mem/allocator.h"
#include "kernel/error.h"
#include "drivers/fb.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t *frames;
uint32_t frames_num;
uint32_t mem_limit;

struct frame_map_addr last_frame[4];

#define BIT_FRAME_SIZE 32
#define MAX_BUDDY_SIZE 8

void fa_test(size_t siz) {
    uintptr_t af[8];
    memset(af, 0, sizeof(af));

    klog("Allocating frames: ");
    fb_print_num(siz);

    if (find_alloc_nframes(siz, af, R_W | PRESENT)) {
        klog("Alloc error");
        return;
    }

    for (unsigned int i = 0; i < 8; i++) {
        /* fb_print_hex(af[i]); */
    }
}

bool is_frame_free(uint32_t frame_map, uint32_t frame) {
    return !(frames[frame_map] & (1 << frame));
}

bool is_free_nframes(size_t nframes, uint32_t frame_map, uint32_t frame) {
    int overflow = 0;
    for (unsigned int i = 0; i < nframes; i++) {
        if (i + frame >= BIT_FRAME_SIZE) {
            frame = 0;
            overflow = -i;
            frame_map ++;
        }
        
        if (!is_frame_free(frame_map, frame + i + overflow)) {
            return false;
        }
    }

    return true;
}

int set_frame_used(uint32_t frame_map, uint32_t frame) {
    if (frames[frame_map] & (1 << frame)) {
        return EINVAL;
    }
    
    frames[frame_map] |= (1 << frame);
    return 0;
}

int set_nframes_used(size_t nframes, uint32_t frame_map, uint32_t frame) {
    int ret = 0;

    int overflow = 0;
    for (unsigned int i = 0; i < nframes; i++) {
        if (frame + i >= BIT_FRAME_SIZE) {
            frame = 0;
            overflow = -i;
            frame_map ++;
        }
        
        ret |= set_frame_used(frame_map, frame + i + overflow);
    }

    return ret;
}

void get_frame_meta(uintptr_t addr,
                    unsigned int *frame_map, unsigned int *frame) {

    *frame_map = addr / (PAGE_SIZE * BIT_FRAME_SIZE);
    *frame = (addr - (*frame_map * PAGE_SIZE * BIT_FRAME_SIZE)) / PAGE_SIZE;
}

static inline uintptr_t frame_map_to_addr(uint32_t fm, uint32_t frame) {
    return ((fm * PAGE_SIZE * BIT_FRAME_SIZE) + (frame * PAGE_SIZE));
}

static inline uint32_t get_frame_submap_mask(uint32_t nframes,
                                             unsigned int *buddy_id) {
    for (*buddy_id = 0; *buddy_id < 4; (*buddy_id)++) {
        if (nframes <= (1u << *buddy_id)) {
            return (0xFFu << (*buddy_id * MAX_BUDDY_SIZE));
        }
    }

    return 0xFFu << (3 * MAX_BUDDY_SIZE);
}

int alloc_nframes(size_t nframes, uintptr_t addr,
                        uintptr_t *alloc_addrs, uint16_t flags) {
    if (!alloc_addrs) {
        return 1;
    }
    
    unsigned int frame_map, frame;
    uint32_t buddy;
    
    get_frame_meta(addr, &frame_map, &frame);
    get_frame_submap_mask(nframes, &buddy);

    int overflow = 0;
    for (unsigned int i = 0; i < nframes; i++) {
        if (frame + i >= BIT_FRAME_SIZE) {
            frame = 0;
            overflow = -i;
            frame_map ++;
        }

        set_frame_used(frame_map, frame + i + overflow);
        alloc_addrs[i] = (addr + (i * PAGE_SIZE)) | flags;
        /* fb_print_hex(alloc_addrs[i]); */
   }

    /*
    last_frame[buddy].frame = frame + nframes + 1;
    last_frame[buddy].frame_map = frame_map;

    if (last_frame[buddy].frame >= BIT_FRAME_SIZE) {
        last_frame[buddy].frame = 0;
        last_frame[buddy].frame_map++;
    }
    */

    return 0;
}

int alloc_frame(uintptr_t addr, uintptr_t *alloc_addr,
                unsigned int flags) {
    return alloc_nframes(1, addr, alloc_addr, flags);
}
static inline void set_nalloc_addrs(uintptr_t *alloc_addrs, uint32_t nframes,
                                    uint32_t fm, uint32_t frame, uint16_t flags) {
    for (unsigned int i = 0; i < nframes; i++) {
        alloc_addrs[i] = frame_map_to_addr(fm, frame + i) | flags;
    }
}

int find_frame_in_map(uint32_t frame_map, uintptr_t *alloc_addr,
                      size_t nframes, unsigned int *map_frames,
                      uint16_t flags) {
    uint32_t buddy = 0;
    uint32_t sub_frame_bitmap = get_frame_submap_mask(nframes, &buddy);

    if ((frames[frame_map] & sub_frame_bitmap) == sub_frame_bitmap) {
        return 1;
    }
    /* fb_print_hex(sub_frame_bitmap); */

    for (unsigned int j = MAX_BUDDY_SIZE * buddy;
         j < MAX_BUDDY_SIZE * (buddy + 1); j += nframes) {

        if (is_frame_free(frame_map, j)) {
            set_nframes_used(nframes, frame_map, j);
            set_nalloc_addrs(alloc_addr, nframes, frame_map, j, flags);

            (*map_frames) += nframes;
            return 0;
        }
    }

    return 1;
}

int find_alloc_nframes(size_t nframes,
                       uintptr_t *alloc_addrs, uint16_t flags) {
    size_t mapped_nof = 0;

    if (!alloc_addrs) {
        return 0;
    }

    for (unsigned int i = 0; i < frames_num; i++) {
        if ((frames[i] & 0xFFFFFFFFu) == 0xFFFFFFFFu) continue;

        uint32_t cur_nof = nframes - mapped_nof;

        if (cur_nof > 8) {
            cur_nof = 8;
        }

        if (find_frame_in_map(i, &alloc_addrs[mapped_nof],
                                cur_nof, &mapped_nof, flags)) {
            continue;
        }

        if (mapped_nof >= nframes) {
            return 0;
        }
    }

    return ENOMEM;
}

int find_alloc_frame(uintptr_t *alloc_addr, unsigned int flags) {
    return find_alloc_nframes(1, alloc_addr, flags);
}

int map_frame(page_table_t pt, unsigned int pte, uint16_t flags) {
    return find_alloc_frame(&pt[pte], flags);
}

void dealloc_nframes(uintptr_t addr, size_t nframes) {
    for (unsigned int i = 0; i < nframes; i++) {
        dealloc_frame(addr + (PAGE_SIZE * i));
    }
}

void dealloc_frame(uintptr_t addr) {
    addr &= ~0xfff; // remove paging flags

    unsigned int frame_map, frame;
    get_frame_meta(addr, &frame_map, &frame);

    frames[frame_map] &= ~(1 << (frame));
}

int frame_alloc_init(size_t pmem_limit) {
    frames_num = pmem_limit / (PAGE_SIZE * BIT_FRAME_SIZE);
    frames = kmalloc(frames_num / BIT_FRAME_SIZE);
    memset(frames, 0, frames_num / BIT_FRAME_SIZE);

    for (unsigned int i = 0; i < ARR_SIZE(last_frame); i++) {
        last_frame[i].frame_map = 0;
        last_frame[i].frame = MAX_BUDDY_SIZE * i;
    }

    if (!frames) {
        return ENOMEM;
    }

    return 0;
}
