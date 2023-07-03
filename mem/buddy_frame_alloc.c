#include "lib/slibc.h"
#include "mem/buddy_alloc.h"
#include "drivers/fb.h"
#include "mem/paging.h"
#include "mem/allocator.h"
#include "mem/frame_allocator.h"
#include "kernel/error.h"
#include "lib/kernel.h"

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

struct free_area free_area[MAX_ORDER];

uint32_t *buddy_maps[MAX_ORDER];

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

static int insert_buddy(struct free_list **fl, order_t order, uintptr_t addr) {
    struct free_list *old_head = (*fl)->next;
    /* klog("a\n"); */
    (*fl)->next = kmalloc(sizeof(**fl));
    if (!(*fl)->next) {
        return ENOMEM;
    }
    
    (*fl)->next->next = old_head;
    (*fl)->next->addr = addr;
    free_area[order].num_free += 1;
    return 0;
}

static inline void remove_free_frame(order_t order, uintptr_t addr) {    
    struct free_area *f_area = &free_area[order];
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
    kfree(*fl);
    (*prev_fl)->next = new_next;
    f_area->num_free -= 1;
}

static inline void remove_free_head(order_t order) {    
    struct free_area *f_area = &free_area[order];
    if (f_area->num_free == 0) {
        return;
    }
    
    struct free_list *new_next = f_area->free_list.next->next;

    /* fb_print_hex((uintptr_t)f_area->free_list.next); */
    kfree(f_area->free_list.next);
    f_area->free_list.next = new_next;
    f_area->num_free --;
}

static inline void set_addrs(uintptr_t *addrs, uintptr_t base,
                             size_t size, uint16_t flags) {
    for (unsigned int i = 0; i < size; i++) {
        addrs[i] = (base + (i * PAGE_SIZE)) | flags;
    }
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

static inline void set_frame_unused(order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy_maps[order][map_id.frame_map] &= ~(1 << map_id.frame);
}

static inline void set_frame_used(order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy_maps[order][map_id.frame_map] |= (1 << map_id.frame);
}

static inline void set_map_used(order_t order, uintptr_t addr,
                             size_t nframes) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);

    for (unsigned int i = 0; i < nframes; i++) {
        buddy_maps[order][map_id.frame_map] |= (1 << (map_id.frame + i));
    }
}

static uintptr_t buddy_slice(uintptr_t addr,
                       order_t start, order_t target) {
    set_frame_used(start, addr);
    
    struct free_list *next_order_fl = &free_area[start - 1].free_list;
    uintptr_t sec_buddy_addr = addr +
                ((PAGE_SIZE * (1 << start)) / 2);

        /* klog("slice\n"); */
    insert_buddy(&next_order_fl, start - 1, sec_buddy_addr);
    /* debug_log("sec buddy addr"); */
    /* fb_print_hex((uintptr_t)sec_buddy_addr); */
   
    if (start - 1 > target) {
        return buddy_slice(addr, start - 1, target);
    }
    
    /* klog("sec bodyy"); */
    /* fb_print_hex(sec_buddy_addr); */
    set_frame_used(target, addr);
    return addr;
}

int buddy_alloc_frames_max_order(uintptr_t *addrs, size_t nframes,
                            uint16_t flags) {

    order_t order = get_buddy_order(nframes);
    struct free_list *free = free_area[order].free_list.next;
    /* fb_print_num(free_area[order].num_free); */

    if (free_area[order].num_free) {
        /* debug_log("free frame available"); */
        remove_free_head(order);
        set_addrs(addrs, free->addr, nframes, flags);
        set_frame_used(order, free->addr);
        return 0;
    }

    for (unsigned int i = order + 1; i < MAX_ORDER; i++) {
        struct free_list *upper_fl = free_area[i].free_list.next;
        debug_log("Search\n");
        struct free_area *fa = &free_area[i];

        if (fa->num_free) {
            klog("Free %d %d\n", i, order);
            remove_free_head(i);
            uintptr_t addr = buddy_slice(upper_fl->addr, i, order);
            set_addrs(addrs, addr, nframes, flags);
            return 0;
        }
    }
    
    klog("Divide frames\n");
    size_t nnof = nframes / 2;
    if (buddy_alloc_frames_max_order(addrs, nnof, flags)) {
        return ENOMEM;
    }

    return buddy_alloc_frames_max_order(addrs + (nnof), nnof, flags);
}

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes, uint16_t flags) {
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

        buddy_alloc_frames_max_order(addrs, cur_nframes, flags);
    }

    return 0;
}

int buddy_get_free_at_addr(struct free_list *init_fl,
                            struct free_list **free,
                           struct free_list **prev_fl,
                           uintptr_t addr) {
    if (!init_fl || !free || !prev_fl) {
        return EINVAL;
    }
    
    struct free_list *fl = init_fl->next;
    *prev_fl = init_fl;
    
    for (; fl; fl = fl->next) {
        if (fl->addr == addr) {
            *free = fl;
            break;
        }
        *prev_fl = fl;
    }

    if (!*free) {
        /* klog("addr: %x\n", fl->addr); */
        return EINVAL;
    }

    return 0;
}
                           

int buddy_alloc_at_addr(uintptr_t base, uintptr_t *addrs, size_t nframes,
                        uint16_t flags) {
    if (!addrs) {
        return EINVAL;
    }

    order_t order = get_buddy_order(nframes);
    struct free_list *init_fl = &free_area[order].free_list;
    /* fb_print_num((uintptr_t)free_area[order].free_list.next); */

    if (free_area[order].num_free > 0) {
        struct free_list *free = NULL;
        struct free_list *prev_fl = NULL;

        if (buddy_get_free_at_addr(init_fl, &free, &prev_fl, base)) {
            return ENOMEM;
        }
        klog("Alloc at addr: %x\n", free->addr);
        
        free_area[order].num_free--;
        prev_fl->next = free->next;
        
        set_addrs(addrs, free->addr, nframes, flags);
        set_frame_used(order, free->addr);
        return 0;
    }

    for (unsigned int i = order + 1; i < MAX_ORDER; i++) {
        struct free_area *fa = &free_area[i];

        if (fa->num_free) {
            struct free_list *free = NULL;
            struct free_list *prev_fl = NULL;

            klog("Iter\n");
            if (buddy_get_free_at_addr(&fa->free_list, &free, &prev_fl, base)) {
                return ENOMEM;
            }

            free_area[i].num_free--;
            prev_fl->next = free->next;
        
            uintptr_t addr = buddy_slice(free->addr, i, order);
            set_addrs(addrs, addr, nframes, flags);
            return 0;
        }
    }
 
    return EINVAL;
}

int buddy_alloc_frame(uintptr_t *addr, uint16_t flags) {
    return buddy_alloc_frames(addr, 1, flags);
}

void buddy_coalesce(order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);
    uint8_t buddy_frame = map.frame + 1;

    if (map.frame % 2) {
        buddy_frame = map.frame - 1;
    }

    struct frame_map_addr buddy_map = map;
    buddy_map.frame = buddy_frame;
 
    set_frame_unused(order, addr);

    if (order < MAX_ORDER - 1 &&
        is_frame_free(buddy_maps[order][map.frame_map], buddy_frame)) {

        /* debug_log("coalesce"); */
        /* fb_print_num(free_area[order].num_free); */
        remove_free_frame(order, map_to_addr(order, buddy_map));

        buddy_coalesce(order + 1, addr);
    } else {
        struct free_list *fl = &free_area[order].free_list;
        insert_buddy(&fl, order, addr);
        /* debug_log("insert order"); */
        /* fb_print_num(free_area[order].num_free); */
    }
}

void buddy_free_frames_rec(order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);

    if (!is_frame_free(buddy_maps[order][map.frame_map], map.frame)) {
        buddy_coalesce(order, addr);
        return;
    }
    
    buddy_free_frames_rec(order + 1, addr);
}

void buddy_free_frames(uintptr_t addr, size_t nframes) {
    order_t order = get_buddy_order(nframes);
    buddy_free_frames_rec(order, addr);
}

void buddy_free_frame(uintptr_t addr) {
    buddy_free_frames(addr, 1);
}

int buddy_alloc_init(size_t mem_limit) {
    if (!mem_limit) {
        return EINVAL;
    }

    memset(&free_area, 0, sizeof(free_area));
    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        size_t map_size = mem_limit / (PAGE_SIZE * MAP_SIZE * (1 << i));
        buddy_maps[i] = kmalloc(map_size);

        if (!buddy_maps[i]) {
            return ENOMEM;
        }
    }

    size_t max_map_size = mem_limit / (MAX_BUDDY_SIZE);
    struct free_list **cur_free = &(free_area[MAX_ORDER - 1].free_list.next);
    free_area[MAX_ORDER - 1].num_free = max_map_size;

    for (unsigned int i = 0; i < max_map_size; i++) {
        *cur_free = kmalloc(sizeof(**cur_free));

        if (!*cur_free) {
            return ENOMEM;
        }

        (*cur_free)->addr = (i * MAX_BUDDY_SIZE);
        /* fb_print_hex((uintptr_t)cur_free); */
        cur_free = &(*cur_free)->next;
    }
    
    return 0;
}

void buddy_test(size_t mem) {
    uintptr_t addrs[1024];
    /* buddy_alloc_init(mem); */
    memset(addrs, 0, sizeof(addrs));
    int ret;

    memset(addrs, 0, sizeof(addrs));
    klog("Buddy find alloc\n");
    ret = buddy_alloc_frames(addrs, 4, R_W|PRESENT);

    for (unsigned int i = 0; i < 4; i ++) {
        fb_print_hex(addrs[i]);
        fb_newline();
    }

    /* buddy_free_frames(addrs[0], 4); */

    klog("Buddy find alloc\n");
    memset(addrs, 0, sizeof(addrs));
    /* klog("Buddy find alloc\n"); */
    ret = buddy_alloc_frames(addrs, 4, R_W|PRESENT);

    for (unsigned int i = 0; i < 4; i ++) {
        fb_print_hex(addrs[i]);
        fb_newline();
    }
    memset(addrs, 0, sizeof(addrs));
    klog("Buddy find alloc\n");
    ret = buddy_alloc_frames(addrs, 8, R_W|PRESENT);

    for (unsigned int i = 0; i < 8; i ++) {
        fb_print_hex(addrs[i]);
        fb_newline();
    }

    buddy_free_frames(addrs[0], 8);

    memset(addrs, 0, sizeof(addrs));
    klog("Buddy find alloc\n");
    ret = buddy_alloc_frames(addrs, 8, R_W|PRESENT);

    for (unsigned int i = 0; i < 8; i ++) {
        fb_print_hex(addrs[i]);
        fb_newline();
    }


    return;
}
