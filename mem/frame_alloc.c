#include "mem/allocator.h"
#include "drivers/fb.h"
#include "mem/frame_allocator.h"
#include "mem/paging.h"
#include "lib/kernel.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

uint32_t *frames;
uint32_t frames_num;
uint32_t mem_limit;

bool is_free_nframes(size_t nframes, uint32_t frame_map, uint32_t frame) {
}

bool is_free_frame(uint32_t frame_map, uint32_t frame) {
    return !(frames[frame_map] & (1 << frame));
}

void get_frame_meta(uintptr_t addr,
                    unsigned int *frame_map, unsigned int *frame) {

    *frame_map = addr / (PAGE_SIZE * sizeof(*frames));
    *frame = (addr - (*frame_map * PAGE_SIZE * sizeof(*frames))) / PAGE_SIZE;
}

uintptr_t alloc_nframes(size_t nframes, uintptr_t addr, uintptr_t *alloc_addrs, uint16_t flags) {
    if (!alloc_addrs) {
        return 1;
    }
    
    if (addr) {
        unsigned int frame_map, frame;
        get_frame_meta(addr, &frame_map, &frame);

        for (unsigned int i = 0; i < nframes; i++) {
            if (i >= sizeof(*frames)) {
                i = 0;
                frame_map ++;
            }

            if (frames[frame_map] & (1 << (frame + i))) {
                return 1;
            }

            alloc_addrs[i] = (addr + (i * PAGE_SIZE)) | flags;
        }
        
        frames[frame_map] |= (1 << frame);

        return 0;
    }
    
    return 1;

}

int alloc_frame(uintptr_t addr, uintptr_t *alloc_addr, unsigned int flags) {
    return alloc_nframes(1, addr, alloc_addr, flags);
}



int find_alloc_nframes(size_t nframes, uintptr_t *alloc_addrs, uint16_t flags) {
    for (unsigned int f = 0; f < nframes; f++) {
        for (unsigned int i = 0; i < frames_num; i++) {
            if ((frames[i] & 0xFFFFFFFF) == 0xFFFFFFFF) continue;

            if (nframes == 1) {
                if ((frames[i] & 0xFFu) == 0xFFu) {
                    continue;
                }

                for (unsigned int j = 0; j < 8; j++) {
                    if (is_free_frame(i, j)) {
                        frames[i] |= (1 << j);
                        alloc_addrs[f] =
                            ((i * PAGE_SIZE * sizeof(*frames)) + (j * PAGE_SIZE));
                        f++;

                        if (f >= nframes) {
                            return 0;
                        }
                    }
                }
            } else if (nframes == 2) {
                if ((frames[i] & 0xFF00u) == 0xFF00u) {
                    continue;
                }

                for (unsigned int j = 8; j < 16; j += 2) {
                    if (is_free_nframes(2, i, j)) {
                        frames[i] |= (1 << j);
                        alloc_addrs[f] =
                            ((i * PAGE_SIZE * sizeof(*frames)) + (j * PAGE_SIZE));
                        f++;

                        if (f >= nframes) {
                            return 0;
                        }
                    }
                }
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

void map_frame(page_table_t pt, unsigned int pte, uint16_t flags) {
    pt[pte] = find_alloc_frame(flags);
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
