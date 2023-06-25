#include "mem/paging.h"
#include "mem/allocator.h"
#include "kernel/error.h"

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
        return MAX_ORDER;
    }
    
    for (uint8_t order = 0; order < MAX_ORDER; order++) {
        if (nframes <= (1u << order)) {
            return order;
        }
    }

    return MAX_ORDER;
}

static int insert_buddy(struct free_list *fl, uintptr_t addr) {
    struct free_list *old_head = fl->next;
    fl->next = kmalloc(sizeof(*fl));
    if (!fl->next) {
        return ENOMEM;
    }
    
    fl->next->next = old_head;
    fl->next->addr = addr;
    return 0;
}

static inline void remove_free_head(order_t order) {
    struct free_area *f_area = &free_area[order];
    kfree(f_area->free_list.next);
    f_area->free_list.next = f_area->free_list.next->next;
    f_area->num_free --;
}

static uintptr_t buddy_slice(struct free_list *free,
                       order_t start, order_t target) {
    remove_free_head(start);
    
    free_area[start - 1].num_free++;
    struct free_list *next_order_fl = &free_area[start - 1].free_list;
    insert_buddy(next_order_fl, free->addr);
    uintptr_t sec_buddy_addr = free->addr +
                ((PAGE_SIZE * (1 << start)) / 2);
   
    if (start - 1 > target) {
        insert_buddy(next_order_fl, sec_buddy_addr);
        return buddy_slice(next_order_fl, start - 1, target);
    }
    
    return sec_buddy_addr;
}

static inline void set_addrs(uintptr_t *addrs, uintptr_t base, size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        addrs[i] = base + (i * PAGE_SIZE);

    }
}

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes) {
    if (!addrs) {
        return EINVAL;
    }

    order_t order = get_buddy_order(nframes);
    struct free_list *free = free_area[order].free_list.next;

    if (free) {
        remove_free_head(order);
        set_addrs(addrs, free->addr, nframes);
        return 0;
    }


    for (unsigned int i = order + 1; i < MAX_ORDER; i++) {
        if (free_area[i].num_free > 0) {
            uintptr_t addr = buddy_slice(&free_area[i].free_list, i, order);
            set_addrs(addrs, addr, nframes);
        }
    }

    return 0;
}

void buddy_free_frame(uintptr_t addr) {

}

int buddy_alloc_init(size_t mem_limit) {
    if (!mem_limit) {
        return EINVAL;
    }
/*
    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        size_t map_size = mem_limit / (PAGE_SIZE * MAP_SIZE * (1 << i));
        buddy_maps[i] = kmalloc(map_size);

        if (!buddy_maps[i]) {
            return ENOMEM;
        }
    }
*/

    size_t max_map_size = mem_limit / (MAX_BUDDY_SIZE * MAP_SIZE);
    struct free_list *cur_free = &free_area[MAX_ORDER - 1].free_list;
    free_area[MAX_ORDER - 1].num_free = max_map_size;
    cur_free = cur_free->next;

    for (unsigned int i = 0; i < max_map_size; i++) {
        cur_free->addr = (i * MAX_BUDDY_SIZE);

        if (!cur_free) {
            return ENOMEM;
        }

        cur_free = cur_free->next;
    }
    
    return 0;
}
