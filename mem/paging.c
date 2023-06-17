#include "mem/paging.h"
#include "drivers/fb.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

#define VADDR 0xC0000000

enum {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER_SUPERVISOR      = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
};

struct kalloc_table {
    bool used;
    unsigned int id;
};

struct virt_addr {
    uintptr_t pde : 10;
    uintptr_t pte : 10;
    uintptr_t p_offset : 12;
} __attribute__((packed));

void load_page_dir(uintptr_t dir);
void enable_paging(void);
void print_cr0(void);

#define KERNEL_INIT_PT_COUNT 4
#define PT_SIZE 1024
#define PAGE_SIZE 4096

static uintptr_t page_dir[1024] __attribute__((aligned(PAGE_SIZE)));

static uintptr_t kernel_page_table[KERNEL_INIT_PT_COUNT * 0x400]
    __attribute__((aligned(PAGE_SIZE)));

extern uintptr_t _kernel_end;
static uintptr_t kernel_end_addr __attribute__((aligned(PAGE_SIZE)));
static uintptr_t pt_base_addr __attribute__((aligned(PAGE_SIZE)));
static uintptr_t heap_base_addr __attribute__((aligned(PAGE_SIZE)));
static struct block_header *heap_base_block
    __attribute__((aligned(PAGE_SIZE)));

static uintptr_t heap_end_addr;

#define BLOCK_HEADER_MAGIC_NUM 0x0BADBABA

struct block_header {
    int magic_num;
    struct block_header *next;
    struct block_header *prev;
    struct block_header *next_hole;
    size_t size;
    uint8_t is_hole;
};

void write_header(struct block_header *head, struct block_header *next,
                        struct block_header *prev,
                        struct block_header *next_hole,
                        size_t size, uint8_t is_hole) {
    if (!head) return;
    
    head->next_hole = next_hole;
    head->is_hole = is_hole;
    head->next = next;
    head->prev = prev;
    head->size = size;
}

void heap_init(void *heap_base) {
    heap_base_block= ((struct block_header *)heap_base);
    
    heap_base_block[0] = (struct block_header) {
        .magic_num = BLOCK_HEADER_MAGIC_NUM,
        .next = NULL,
        .prev = NULL,
        .next_hole = NULL,
        .is_hole = true,
        .size = heap_end_addr - (uintptr_t)heap_base,
    };
}
  
void paging_init() {
    for (unsigned int i = 0; i < ARR_SIZE(page_dir); i++) {
        page_dir[i] |= R_W;
    }

    for (unsigned int i = 0; i < ARR_SIZE(kernel_page_table); i++) {
        kernel_page_table[i] = (i * 0x1000) | PRESENT | USER_SUPERVISOR;
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        page_dir[i] = (uintptr_t)&kernel_page_table[PT_SIZE * i] | 0x3;
    }
    
    asm volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    pt_base_addr = kernel_end_addr + 0x1000;
    heap_base_addr = pt_base_addr + (ARR_SIZE(page_dir)*PT_SIZE*PAGE_SIZE);

    load_page_dir((uintptr_t)&page_dir);
    enable_paging();
    add_isr_handler(14, &page_fault, 0);
    heap_init((void *)heap_base_addr);

    klog("Paging enabled\n");
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

static inline uintptr_t get_page_addr(uint16_t pde, uint16_t pte) {
    return (pde * 0x400000) + (pte * 0x1000);
}

int map_page(uintptr_t *pt_addr, uint16_t pde, uint16_t pte) {
    pt_addr[pte] = get_page_addr(pde, pte) | 0x3;

    return pte;
}

int pt_present(uint16_t pde) {
    return tab_present(page_dir[pde]);
}

int page_present(uint16_t pde, uint16_t pte) {
    return tab_present(((uintptr_t *)page_dir[pde])[pte]);
}

int map_pt(uint16_t pde) {
    uintptr_t table_addr = pt_base_addr + (pde * 0x1000);

    for (unsigned int i = 0; i < 1024; i++) {
        map_page((uintptr_t *)table_addr, pde, i);
    }

    klog("Alloc page table\n"); 
    page_dir[pde] = table_addr | 0x3;

    return pde;
 }

int non_present_page_hanler(uint16_t pde, uint16_t pte) {
    if (!pt_present(pde)) {
        map_pt(pde);
    } else if (!page_present(pde, pte)) {
        map_page((uintptr_t *)page_dir[pde], pde, pte);
    }

    return 1;
}

void *kalloc(size_t siz) {
    if (siz == 0) return NULL;

    struct block_header *header;
    
    for (header = heap_base_block;
         header->is_hole;
         header = header->next_hole) {

        if (header->size >= siz) {
            uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
            header->is_hole = false;
            header->size = siz + sizeof(*header);

            if (!header->next || header->size >= siz + 2*sizeof(*header)) {
                struct block_header *new_block =
                    (void *)(data_base + siz);

                header->next_hole = new_block;
                header->next = new_block;

                if (!heap_base_block->next_hole
                    || heap_base_block->next_hole == header) {
                    heap_base_block->next_hole = new_block;
                }

                *new_block = (struct block_header) {
                    .magic_num = BLOCK_HEADER_MAGIC_NUM,
                    .is_hole = true,
                    .size = heap_end_addr - data_base - siz,
                    .prev = header,
                    .next = header->next,
                    .next_hole = header->next_hole,
                };
            }

            return (void *)data_base;
        }
    }

    heap_end_addr += PAGE_SIZE;

    header = (struct block_header *)(void *)
        ((uintptr_t)header + header->size + sizeof(*header));
        
    uintptr_t data_base = ((uintptr_t)header + sizeof(*header));
        struct block_header *new_block =
        (void *)(data_base + siz);

    header->next_hole = new_block;
    header->next = new_block;
    header->is_hole = false;
    header->size = siz + sizeof(*header);


    if (!heap_base_block->next_hole
        || heap_base_block->next_hole == header) {
        heap_base_block->next_hole = new_block;
    }

    *new_block = (struct block_header) {
        .magic_num = BLOCK_HEADER_MAGIC_NUM,
        .is_hole = true,
        .size = heap_end_addr - data_base - siz,
        .prev = header,
        .next = header->next,
        .next_hole = header->next_hole,
    };

    return NULL;
}

static inline void *get_block_header_addr(void *addr) {
    return (void *)((uintptr_t)addr - sizeof(struct block_header));
}

void kfree(void *addr) {
    if (!addr) return;

    if ((uintptr_t)addr > heap_end_addr || (uintptr_t)addr < heap_base_addr) {
        return;
    }
    
    void *block_addr = get_block_header_addr(addr);
    struct block_header *header = (struct block_header *)block_addr;
    
    if (header->magic_num == BLOCK_HEADER_MAGIC_NUM) {
        header->is_hole = true;

        if ((uintptr_t)(void *)heap_base_block->next_hole >
            (uintptr_t)(void *)header) {
            header->next_hole = heap_base_block->next_hole;
            heap_base_block->next_hole = header;
        }
    }
}

void page_fault(struct isr_handler_args args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    klog_warn("Page fault\n");
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    pde = fault_addr >> 22;
    pte = (fault_addr >> 12) & 0x3ff;

    if (args.error ^ PRESENT) {
        non_present_page_hanler(pde, pte);
    } else if (args.error ^ R_W) {

    }
        
    fb_print_num(args.error);
}

