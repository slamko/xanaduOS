#include "mem/allocator.h"
#include "drivers/fb.h"
#include "mem/frame_allocator.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

uint32_t *frames;
uint32_t frames_num;
uint32_t mem_limit;

#define MAX_BUDDY_SIZE 8

bool is_free_frame(uint32_t frame_map, uint32_t frame) {
    return !(frames[frame_map] & (1 << frame));
}

bool is_free_nframes(size_t nframes, uint32_t frame_map, uint32_t frame) {
    for (unsigned int i = 0; i < nframes; i++) {
        if (i + frame >= sizeof(*frames)) {
            frame = 0;
            i = 0;
            frame_map ++;
        }
        
        if (!is_free_frame(frame_map, frame + i)) {
            return false;
        }
    }

    return true;
}

int set_frame_used(uint32_t frame_map, uint32_t frame) {
    if (frames[frame_map] & (1 << frame)) {
        return 1;
    }
    
    frames[frame_map] |= (1 << frame);
    return 0;
}

int set_nframes_used(size_t nframes, uint32_t frame_map, uint32_t frame) {
    int ret = 0;

    for (unsigned int i = 0; i < nframes; i++) {
        ret |= set_frame_used(frame_map, frame + i);
    }

    return ret;
}

void get_frame_meta(uintptr_t addr,
                    unsigned int *frame_map, unsigned int *frame) {

    *frame_map = addr / (PAGE_SIZE * sizeof(*frames));
    *frame = (addr - (*frame_map * PAGE_SIZE * sizeof(*frames))) / PAGE_SIZE;
}

static inline uintptr_t frame_map_to_addr(uint32_t fm, uint32_t frame) {
    return ((fm * PAGE_SIZE * sizeof(*frames)) + (frame * PAGE_SIZE));
}

uintptr_t alloc_nframes(size_t nframes, uintptr_t addr,
                        uintptr_t *alloc_addrs, uint16_t flags) {
    if (!alloc_addrs) {
        return 1;
    }
    
    if (addr) {
        unsigned int frame_map, frame;
        get_frame_meta(addr, &frame_map, &frame);

        for (unsigned int i = 0; i < nframes; i++) {
            if (frame + i >= sizeof(*frames)) {
                frame = i = 0;
                frame_map ++;
            }

            if (set_frame_used(frame_map, frame + i)) {
                return 1;
            }

            alloc_addrs[i] = (addr + (i * PAGE_SIZE)) | flags;
        }
        

        return 0;
    }
    
    return 1;

}

int alloc_frame(uintptr_t addr, uintptr_t *alloc_addr,
                unsigned int flags) {
    return alloc_nframes(1, addr, alloc_addr, flags);
}

static inline uint32_t get_frame_submap_mask(uint32_t nframes) {
    for (unsigned int i = 0; i < MAX_BUDDY_SIZE; i++) {
        if (nframes <= (1u << i)) {
            return (0xFFu << i);
        }
    }

    return 0xFFu << 3;
}

int find_frame_in_map(uint32_t frame_map, uintptr_t *alloc_addr,
                      size_t nframes, unsigned int *map_frames, uint16_t flags) {
    uint32_t sub_frame_bitmap = get_frame_submap_mask(nframes);

    if ((frames[frame_map] & sub_frame_bitmap) == sub_frame_bitmap) {
        return 1;
    }

    for (unsigned int j = 8 * (nframes - 1); j < 8 * nframes; j += nframes) {
        if (is_free_nframes(nframes, frame_map, j)) {
            set_nframes_used(nframes, frame_map, j);
            *alloc_addr = frame_map_to_addr(frame_map, j) | flags;

            (*map_frames)++;

            if (*map_frames >= nframes) {
                return 0;
            }
        }
    }

    return 1;
}

int find_alloc_nframes(size_t nframes,
                       uintptr_t *alloc_addrs, uint16_t flags) {
    uint16_t max_nf = (nframes / 8);
    size_t mapped_nof = 0;

    if (max_nf % 8) {
        max_nf++;
    }
    
    for (unsigned int f = 0; f < max_nf; f++) {
        uint16_t cur_nof = (nframes % 8);

        if (nframes >= (f + 1) * 8) {
            cur_nof = 8;
        }

        for (unsigned int i = 0; i < frames_num; i++) {
            if ((frames[i] & 0xFFFFFFFFu) == 0xFFFFFFFFu) continue;

            if (!find_frame_in_map(i, alloc_addrs+(i * 4),
                                   cur_nof, &mapped_nof, flags)) {
                return 0;
            }
        }
    }

    return 1;
}

int find_alloc_frame(uintptr_t *alloc_addr, unsigned int flags) {
    return find_alloc_nframes(1, alloc_addr, flags);
}

uintptr_t alloc_pt(page_table_t *new_pt, uint16_t flags) {
    uintptr_t phys_addr = 0;
    *new_pt = kmalloc_align_phys(PAGE_SIZE, PAGE_SIZE, &phys_addr);
    memset(*new_pt, 0, PAGE_SIZE);

    return phys_addr | flags;
}

int map_alloc_pt(struct page_dir *pd, page_table_t *pt, uint16_t pde) {
    if (!pd) {
        return 1;
    }

    pd->page_tables[pde] = alloc_pt(pt, R_W | PRESENT);
    if (!pd->page_tables[pde]) {
        return 1;
    }
    
    pd->page_tables_virt[pde] = to_uintptr(pt);
    return 0;
}

int map_frame(page_table_t pt, unsigned int pte, uint16_t flags) {
    return find_alloc_frame(&pt[pte], flags);
}

int map_pt_ident(struct page_dir *page_dir, uint16_t pde, uint16_t flags) {
    page_table_t pt;
    page_dir->page_tables[pde] = alloc_pt(&pt, flags);
    page_dir->page_tables_virt[pde] = to_uintptr(pt);
    
    for (unsigned int i = 0; i < PT_SIZE; i++) {
        map_frame(pt, i, flags);
    }

    return pde;
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
    frames_num = mem_limit / PAGE_SIZE;
    frames = kmalloc(frames_num / sizeof(*frames));
}
