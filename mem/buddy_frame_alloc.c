#include "mem/buddy_alloc.h"
#include "mem/paging.h"
#include "mem/frame_allocator.h"
#include "mem/allocator.h"
#include "lib/slibc.h"
#include "drivers/fb.h"
#include "drivers/initrd.h"
#include "kernel/error.h"
#include "lib/kernel.h"
#include "mem/slab_allocator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_ORDER           11
#define MAX_BUDDY_SIZE      0x400000 
#define MAP_SIZE            32
#define MAX_BUDDY_NFRAMES   1024

typedef uint8_t order_t;

struct free_list {
    struct free_list *next;
    uintptr_t addr;
};

struct free_area {
    struct free_list free_list;
    size_t num_free;
};

struct buddy_alloc {
    size_t mem_size;
    size_t mem_start;
    struct slab_cache *fl_slab;
    struct free_area *free_area;
    uint32_t **maps;
};

static inline uint8_t get_buddy_order(size_t nframes) {
    if (nframes > MAX_BUDDY_NFRAMES) {
        return MAX_ORDER - 1;
    }
    
    for (uint8_t order = 0; order < MAX_ORDER; order++) {
        if (nframes <= (1u << order)) {
            return order;
        }
    }

    return MAX_ORDER - 1;
}

static inline bool is_frame_free(uint32_t bitmap, uint8_t frame) {
    return !(bitmap & (1 << frame));
}

static int insert_buddy(struct buddy_alloc *buddy,
                        struct free_list *fl, order_t order, uintptr_t addr) {
    if (!fl) {
        return EINVAL;
    }
    struct free_list *old_head = fl->next;
    fl->next = slab_alloc_from_cache(buddy->fl_slab);
        /* fl->next = kmalloc(sizeof(*fl->next)); */

    if (!fl->next) {
        klog_error("Slab allocation failed %x\n", fl->next);
        return ENOMEM;
    }
    
    fl->next->next = old_head;
    fl->next->addr = addr;
    buddy->free_area[order].num_free += 1;
    return 0;
}

static inline void remove_free_frame(struct buddy_alloc *buddy,
                                     order_t order, uintptr_t addr) {    
    struct free_area *f_area = &buddy->free_area[order];
    if (f_area->num_free == 0) {
        return;
    }

    struct free_list *init_fl = &f_area->free_list;
    struct free_list **prev_fl = &init_fl;
    struct free_list **fl = &(init_fl->next);

    for (; (*fl); fl = &(*fl)->next) {
        if ((*fl)->addr + (PAGE_SIZE * (1 << order)) > addr &&
            (*fl)->addr <= addr) {
            /* klog("remove"); */
            break;
        }
        prev_fl = fl;
    }
    
    struct free_list *new_next = (*fl)->next;

    /* fb_print_hex((uintptr_t)f_area->free_list.next); */
    slab_free(buddy->fl_slab, *fl);
    /* kfree(*fl); */
    (*prev_fl)->next = new_next;
    f_area->num_free -= 1;
}

static inline void remove_free_head(struct buddy_alloc *buddy, order_t order) {    
    struct free_area *f_area = &buddy->free_area[order];
    if (f_area->num_free == 0) {
        return;
    }
    
    struct free_list *new_next = f_area->free_list.next ? f_area->free_list.next->next : NULL;

    
    slab_free(buddy->fl_slab, f_area->free_list.next);
    /* klog("Before slab free %x\n", f_area->free_list.next); */
    f_area->free_list.next = new_next;
    f_area->num_free --;
}

static inline struct frame_map_addr addr_to_map_id(order_t order,
                                                   uintptr_t addr) {
    struct frame_map_addr map;
    map.frame_map = addr / (PAGE_SIZE * MAP_SIZE * (1 << order));
    map.frame =
        (addr - (map.frame_map * (PAGE_SIZE * MAP_SIZE * (1 << order)))) /
        ((1 << order) * PAGE_SIZE);

    return map;
}

static inline uintptr_t map_to_addr(order_t order,
                                    struct frame_map_addr map) {
    uintptr_t base = (map.frame_map * MAP_SIZE * PAGE_SIZE * (1 << order));
    uintptr_t frame_addr = map.frame * PAGE_SIZE * (1 << order);
    return base + frame_addr;
}

static inline void set_frame_unused(struct buddy_alloc *buddy,
                                    order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy->maps[order][map_id.frame_map] &= ~(1 << map_id.frame);
}

static inline void set_frame_used(struct buddy_alloc *buddy,
                                  order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy->maps[order][map_id.frame_map] |= (1 << map_id.frame);
}

static inline void set_map_used(struct buddy_alloc *buddy,
                                order_t order, uintptr_t addr, size_t nframes) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);

    for (unsigned int i = 0; i < nframes; i++) {
        buddy->maps[order][map_id.frame_map] |= (1 << (map_id.frame + i));
    }
}

static int buddy_slice(struct buddy_alloc *buddy, uintptr_t addr,
                             uintptr_t *res, order_t start, order_t target) {
    set_frame_used(buddy, start, addr);
   
    /* debug_log("Buddy addr %x\n", addr); */
    struct free_list *next_order_fl = &buddy->free_area[start - 1].free_list;
    uintptr_t sec_buddy_addr = addr +
                ((PAGE_SIZE * (1 << start)) / 2);

    if (insert_buddy(buddy, next_order_fl, start - 1, sec_buddy_addr)) {
        return 1;
    }

    if (start - 1 > target) {
        return buddy_slice(buddy, addr, res, start - 1, target);
    }
    
    set_frame_used(buddy, target, addr);
    *res = addr;
    return 0;
}

int buddy_alloc_frames_max_order(struct buddy_alloc *buddy, uintptr_t *addrs,
                                 size_t nframes, uint16_t flags) {

    order_t order = get_buddy_order(nframes);
    struct free_list *free = buddy->free_area[order].free_list.next;

    if (buddy->free_area[order].num_free) {
        /* debug_log("Free frame available\n"); */
        remove_free_head(buddy, order);
        *addrs = free->addr;
        set_frame_used(buddy, order, free->addr);
        return 0;
    }

    for (unsigned int i = order + 1; i < MAX_ORDER; i++) {
        struct free_list *upper_fl = buddy->free_area[i].free_list.next;
        struct free_area *fa = &buddy->free_area[i];

        /* debug_log("Search\n"); */
        if (fa->num_free) {
            if (!fa->free_list.next) {
                fa->num_free = 0;
                continue;
            }
            
            /* klog("Found buddy %d\n", fa->num_free); */
            remove_free_head(buddy, i);
            uintptr_t addr = 0;

            if (buddy_slice(buddy, upper_fl->addr, &addr, i, order)) {
                return 1;
            }
            *addrs = addr;

            /* klog("Alloc addr %d\n", addrs[0]); */
            return 0;
        }
    }
    
    return ENOMEM;
}

int buddy_alloc_frames(struct buddy_alloc *buddy,
                       uintptr_t *addrs, size_t nframes, uint16_t flags) {
    int ret;

    if (!addrs) {
        return EINVAL;
    }

    unsigned long rep_limit = nframes / MAX_BUDDY_NFRAMES;
    if (nframes % MAX_BUDDY_NFRAMES) {
        rep_limit ++;
    }
    
    for (unsigned int i = 0; i < rep_limit; i++) {
        size_t cur_nframes = nframes - (i * MAX_BUDDY_NFRAMES);

        if (cur_nframes > MAX_BUDDY_NFRAMES) {
            cur_nframes = MAX_BUDDY_NFRAMES;
        }

        klog("Current number of frames %d\n", cur_nframes);
        ret = buddy_alloc_frames_max_order(buddy,
                                           addrs + (i * MAX_BUDDY_NFRAMES),
                                           cur_nframes, flags);

        if (ret) {
            return ret;
        }
    }

    return 0;
}

int buddy_alloc_frame(struct buddy_alloc *buddy, uintptr_t *addr, uint16_t flags) {
    return buddy_alloc_frames(buddy, addr, 1, flags);
}

void buddy_coalesce(struct buddy_alloc *buddy, order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);
    uint8_t buddy_frame = map.frame + 1;

    if (map.frame % 2) {
        buddy_frame = map.frame - 1;
    }

    struct frame_map_addr buddy_map = map;
    buddy_map.frame = buddy_frame;
 
    set_frame_unused(buddy, order, addr);

    if (order < MAX_ORDER - 1 &&
        is_frame_free(buddy->maps[order][map.frame_map], buddy_frame)) {

        /* debug_log("coalesce"); */
        remove_free_frame(buddy, order, map_to_addr(order, buddy_map));

        buddy_coalesce(buddy, order + 1, addr);
    } else {
        struct free_list *fl = &buddy->free_area[order].free_list;
        insert_buddy(buddy, fl, order, addr);
        /* debug_log("insert order"); */
    }
}

void buddy_free_frames_rec(struct buddy_alloc *buddy,
                           order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);

    if (!is_frame_free(buddy->maps[order][map.frame_map], map.frame)) {
        buddy_coalesce(buddy, order, addr);
        return;
    }
    
    buddy_free_frames_rec(buddy, order + 1, addr);
}

void buddy_free_frames(struct buddy_alloc *buddy, uintptr_t addr, size_t nframes) {
    order_t order = get_buddy_order(nframes);
    buddy_free_frames_rec(buddy, order, addr);
}

void buddy_free_frame(struct buddy_alloc *buddy, uintptr_t addr) {
    buddy_free_frames(buddy, addr, 1);
}

size_t get_buddy_map_size(size_t mem_size) {
    size_t map_size = 0;

    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        size_t map_factor = (PAGE_SIZE * MAP_SIZE * (1 << i));
        map_size += mem_size / map_factor;

        if (mem_size % map_factor) {
            map_size++;
        }
    }
    return map_size;
}

static int alloc_buddy_maps(struct buddy_alloc *buddy, size_t map_size) {
    
    buddy->maps = kmalloc((sizeof(*buddy->maps) * MAX_ORDER));
    uint32_t *map_data = kmalloc(map_size);

    if (!buddy->maps) {
        return ENOMEM;
    }

    size_t map_offset = 0;
    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        buddy->maps[i] = &map_data[MAX_ORDER + map_offset];
        /* klog("Buddy map addr %x\n", buddy->maps[i]); */
        map_offset +=
            buddy->mem_size
            / (PAGE_SIZE * MAP_SIZE * (1 << i) * sizeof(map_data));
    }

    return 0;
}

struct buddy_alloc *buddy_alloc_create(size_t mem_start, size_t mem_limit) {
    size_t mem_size = mem_limit - mem_start;
    struct buddy_alloc *buddy;
    
    if (!mem_limit) {
        return NULL;
    }

    buddy = kmalloc(sizeof *buddy);
    buddy->fl_slab = slab_cache_create(sizeof(struct free_list));
    buddy->mem_size = mem_size;
    buddy->mem_start = mem_start;

    size_t map_size = get_buddy_map_size(mem_size) * sizeof(**buddy->maps);
    alloc_buddy_maps(buddy, map_size);
    
    buddy->free_area = kzalloc(0, sizeof(struct free_area) * MAX_ORDER);
    size_t max_map_size = mem_size / (MAX_BUDDY_SIZE);
    struct free_area *last_fa = &buddy->free_area[MAX_ORDER - 1];
    struct free_list **cur_free = &(last_fa->free_list.next);
    buddy->free_area[MAX_ORDER - 1].num_free = max_map_size;

    for (unsigned int i = 0; i < max_map_size; i++) {
        *cur_free = slab_alloc_from_cache(buddy->fl_slab);
        /* *cur_free = kmalloc(sizeof(**cur_free)); */

        if (!*cur_free) {
            /* klog("Ret null\n"); */
            return NULL;
        }

        (*cur_free)->addr = mem_start + (i * MAX_BUDDY_SIZE);
        /* klog("Addrs %x\n", (*cur_free)->addr); */
        cur_free = &(*cur_free)->next;
    }
    
    return buddy;
}

struct buddy_alloc *buddy_alloc_clone(struct buddy_alloc *copy) {
    struct buddy_alloc *buddy;

    buddy = kmalloc(sizeof *buddy);
    buddy->fl_slab = slab_cache_create(sizeof(struct free_list));
    buddy->mem_size = copy->mem_size;
    buddy->mem_start = copy->mem_start;
    
    size_t map_size = get_buddy_map_size(copy->mem_size) * sizeof(**buddy->maps);
    klog("Map size %x\n", map_size);
    alloc_buddy_maps(buddy, map_size);

    size_t map_base_off = sizeof(*buddy->maps) * MAX_ORDER;
    klog("Map base off%x\n", map_base_off);
    /* void *new = kmalloc(map_size); */
    for (size_t i = 0;i < map_size/sizeof(*buddy->maps); i++) {
        /* fb_print_hex(buddy->maps[0][i]); */
    }
    memcpy(buddy->maps[0], copy->maps[0], map_size);

    klog("Buddy clone maps %u\n", map_size);
    buddy->free_area = kzalloc(0, sizeof(struct free_area) * MAX_ORDER);
    size_t max_map_size = copy->mem_size / (MAX_BUDDY_SIZE);
    buddy->free_area[MAX_ORDER - 1].num_free = max_map_size;
    klog("Buddy clone: allocated free area\n");

    for (size_t i = 0; i < MAX_ORDER; i++) {
        size_t num_free = copy->free_area[i].num_free;
        buddy->free_area[i].num_free = num_free;
        struct free_list **new_list = &buddy->free_area[i].free_list.next;
        struct free_list *copy_list = copy->free_area[i].free_list.next;
        
        foreach(copy_list,
                *new_list = slab_alloc_from_cache(buddy->fl_slab);
                (*new_list)->addr = copy_list->addr;
                new_list = &(*new_list)->next;
            );
    }
 
    return buddy;
}

void buddy_alloc_clean(struct buddy_alloc *buddy) {
    size_t map_size = get_buddy_map_size(buddy->mem_size);
    size_t map_base_off = (sizeof(*buddy->maps) * MAX_ORDER);

    memset(buddy->maps + map_base_off, 0, map_size);

    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        struct free_area *fa = &buddy->free_area[i];

        if (fa->num_free) {
            struct free_list *fl_to_free = fa->free_list.next;
            for (struct free_list *fl = fl_to_free->next;
                 fl;
                 fl = fl->next) {

                slab_free(buddy->fl_slab, fl_to_free);
                fl_to_free = fl;
            }
        }
        fa->num_free = 0;
    }

    size_t max_map_size = buddy->mem_size / (MAX_BUDDY_SIZE);
    struct free_area *last_fa = &buddy->free_area[MAX_ORDER - 1];
    struct free_list **cur_free = &(last_fa->free_list.next);
    buddy->free_area[MAX_ORDER - 1].num_free = max_map_size;

    for (unsigned int i = 0; i < max_map_size; i++) {
        *cur_free = slab_alloc_from_cache(buddy->fl_slab);
        /* *cur_free = kmalloc(sizeof(**cur_free)); */

        if (!*cur_free) {
            break;
        }

        (*cur_free)->addr = buddy->mem_start + (i * MAX_BUDDY_SIZE);
        /* klog("Addrs %x\n", (*cur_free)->addr); */
        cur_free = &(*cur_free)->next;
    }
    
}


void buddy_alloc_destroy(struct buddy_alloc *buddy) {
    if (!buddy) {
        return;
    }
    
    kfree(buddy->maps);

    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        struct free_area *fa = &buddy->free_area[i];

        if (fa->num_free) {
            struct free_list *fl_to_free = fa->free_list.next;
            for (struct free_list *fl = fl_to_free->next;
                 fl;
                 fl = fl->next) {

                slab_free(buddy->fl_slab, fl_to_free);
                fl_to_free = fl;
            }
        }
    }
    
    slab_cache_destroy(buddy->fl_slab);
    kfree(buddy->free_area); 
    kfree(buddy);
}

void buddy_test(size_t mem) {
    struct buddy_alloc *buddy = buddy_alloc_create(0x100000, 0x40000000);
    uintptr_t addrs[0x04];
    if (buddy_alloc_frames(buddy, addrs, 1, 0)) {
        klog("ALloc error\n");
    }

    for (unsigned int i = 0; i < 1; i ++) {
        klog("Buddy alloc at %x\n", addrs[i]);
    }
    
    if (buddy_alloc_frames(buddy, addrs, 1, 0)) {
        klog("ALloc error\n");
    }

    for (unsigned int i = 0; i < 1; i ++) {
        klog("Buddy alloc at %x\n", addrs[i]);
    }

    buddy_free_frame(buddy, 0x100000);
     
    buddy_alloc_frames(buddy, addrs, 1, 0);

    for (unsigned int i = 0; i < 1; i ++) {
        klog("Buddy alloc at %x\n", addrs[i]);
    }
    
    return;
}
