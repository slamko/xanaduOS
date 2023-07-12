#include "mem/paging.h"
#include "drivers/initrd.h"
#include "mem/allocator.h"
#include "mem/mmap.h"
#include "mem/slab_allocator.h"
#include "mem/buddy_alloc.h"
#include "kernel/error.h"
#include "mem/frame_allocator.h"
#include "drivers/fb.h"
#include "kernel/error.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include "drivers/int.h"
#include "lib/kernel.h"
#include "lib/slibc.h"

#define VADDR 0xC0000000

struct virt_addr {
    uintptr_t pde : 10;
    uintptr_t pte : 10;
    uintptr_t p_offset : 12;
} __attribute__((packed));

void page_fault(struct isr_handler_args *args);

void load_page_dir(uintptr_t dir);
void enable_paging(void);
void disable_paging(void);

#define KERNEL_INIT_PT_COUNT 4
#define DEFAULT_FLAGS (R_W | PRESENT)
#define MAP_LOWER_ADDR 0x100000

uintptr_t map_limit;

struct page_dir init_pd;
struct page_dir *cur_pd;
struct page_dir *kernel_pd;

static uintptr_t init_page_tables[PT_SIZE] __attribute__((aligned(PAGE_SIZE)));
static page_table_t init_page_tables_virt[PT_SIZE]; 

static uintptr_t kernel_page_table[KERNEL_INIT_PT_COUNT * PT_SIZE]
    __attribute__((aligned(PAGE_SIZE)));

static uintptr_t virt_kernel_addr;
static uintptr_t rodata_start;
static uintptr_t rodata_end;
static uintptr_t kernel_end_addr __attribute__((aligned(PAGE_SIZE)));

static uintptr_t pt_base_addr __attribute__((aligned(PAGE_SIZE)));
static uintptr_t last_fault_addr;

static struct slab_cache *slab_cache;

uintptr_t to_phys_addr(const struct page_dir *pd, uintptr_t virt_addr) {
    pte_t pde;
    pte_t pte;
    pte_t page_offset = virt_addr & 0xfff;

    if (!pd) {
        return virt_addr -
            (virt_kernel_addr ? virt_kernel_addr : VADDR);
    }
    
    get_pde_pte(virt_addr, &pde, &pte);
    if (!pd->page_tables_virt[pde]) {
        klog_error("Requested virtual address not mapped %x\n", virt_addr);
        return 0;
    }
    
    uintptr_t phys_addr = pd->page_tables_virt[pde][pte];
    phys_addr = get_tab_pure_addr(phys_addr) + page_offset;

    /* klog("Pddd: %x\n", cur_pd->page_tables_virt[768]); */
    return phys_addr;
}


uintptr_t ptr_to_phys_addr(const struct page_dir *pd, void *ptr) {
    return to_phys_addr(pd, to_uintptr(ptr));
}

void flush_pages(uintptr_t *virt_addr, size_t npages) {
    for (unsigned int i = 0; i < npages; i++) {
        flush_page(virt_addr[i]);
    }
}

void flush_pages_contiguous(uintptr_t virt_addr, size_t npages) {
    for (unsigned int i = 0; i < npages; i++) {
        flush_page(virt_addr + (i * 0x1000));
    }
}

int page_tables_init(void) {
    int ret;
    
    for (unsigned int i = 0; i < ARR_SIZE(init_page_tables); i++) {
        init_pd.page_tables[i] |= R_W;
    }

    for (unsigned int i = 0x0; i < KERNEL_INIT_PT_COUNT * PT_SIZE; i++) {
        uintptr_t paddr = i * 0x1000;
        uint16_t flags =  R_W | PRESENT;

        /* if (paddr >= rodata_end || paddr < rodata_start) { */
            /* flags |= R_W; */
        /* } */
        
        alloc_frame(paddr, kernel_page_table + i, flags);
    }

    for (unsigned int i = 0; i < KERNEL_INIT_PT_COUNT; i++) {
        init_pd.page_tables[768 + i] =
            ptr_to_phys_addr(cur_pd, &kernel_page_table[PT_SIZE * i])
            | PRESENT
            | R_W
            | GLOBAL
            ;

        init_pd.page_tables_virt[768 + i] = &kernel_page_table[PT_SIZE * i];
    }

    ret = add_isr_handler(14, &page_fault, 0);

    load_page_dir(init_pd.pd_phys_addr);
    enable_paging();

    return ret;
}

void paging_init(size_t pmem_limit) {
    __asm__ volatile ("mov $_kernel_end, %0" : "=r" (kernel_end_addr));
    __asm__ volatile ("mov $_virt_kernel_addr, %0" : "=r" (virt_kernel_addr));
    __asm__ volatile ("mov $_rodata_start, %0" : "=r" (rodata_start));
    __asm__ volatile ("mov $_rodata_end, %0" : "=r" (rodata_end));

    int ret;
    rodata_start = to_phys_addr(cur_pd, rodata_start);
    rodata_end = to_phys_addr(cur_pd, rodata_end);

    pt_base_addr = kernel_end_addr + (PAGE_SIZE * 1);
    map_limit = virt_kernel_addr +
        (KERNEL_INIT_PT_COUNT * PT_SIZE * PAGE_SIZE);

    heap_init(pt_base_addr);
    slab_alloc_init(pt_base_addr);
    ret = frame_alloc_init(pmem_limit);

    if (ret) {
        panic("Frame allocator initialization failed\n", ret);
    }

    init_pd.page_tables_virt = init_page_tables_virt;
    init_pd.page_tables = init_page_tables;
    init_pd.pd_phys_addr = to_phys_addr(cur_pd, to_uintptr(&init_page_tables));

    ret = page_tables_init();
    kernel_pd = &init_pd;
    cur_pd = kernel_pd;

    if (ret) {
        panic("Paging data structures initialization failed\n", ret);
    }

    slab_cache = slab_cache_create_align(PAGE_SIZE, PAGE_SIZE);
    /* klog("Cache: %x\n", slab_cache); */
    

    klog("Paging enabled\n");
}

uintptr_t alloc_pt(page_table_t *new_pt, uint16_t flags) {
    uintptr_t phys_addr;
    *new_pt = slab_alloc_from_cache(slab_cache);
    /* *new_pt = kmalloc_align(PAGE_SIZE, PAGE_SIZE); */
    phys_addr = ptr_to_phys_addr(cur_pd, *new_pt);
    memset(*new_pt, 0x0, PAGE_SIZE);

    return phys_addr | flags;
}

int map_alloc_npt(struct page_dir *pd, page_table_t *pt, size_t npt,
                  pte_t pde, uint16_t flags) {
    if (!pd || !pt) {
        return EINVAL;
    }

    for (size_t n = 0; n < npt; n++) {
        size_t cur_pde = pde + n;

        if (pd->page_tables[cur_pde] & ACCESSED) {
            pd->page_tables[cur_pde] &= ~ACCESSED;
        }

        if (tab_present(pd->page_tables[cur_pde])) {
            klog("Page Table already mapped %x\n", pd->page_tables[cur_pde]);
            pd->page_tables[cur_pde] |= flags;
            pt[n] = pd->page_tables_virt[cur_pde];
            return 0;
        }

        pd->page_tables[cur_pde] = alloc_pt(&pt[n], flags);
        if (!pd->page_tables[cur_pde]) {
            return ENOMEM;
        }

        klog("Allocating page table %u\n", cur_pde - 1);
        pd->page_tables_virt[cur_pde] = *pt;
    }

    return 0;
}

int map_alloc_pt(struct page_dir *pd, page_table_t *pt,
                 pte_t pde, uint16_t flags) {
    return map_alloc_npt(pd, pt, 1, pde, flags);
}

int copy_page(struct page_dir *pd, uintptr_t dest_phys, uintptr_t src_virt) {
    uintptr_t v_map_addr;

    if (kmmap(kern_buddy, pd, &v_map_addr, dest_phys, R_W | PRESENT)) {
        return 1;
    }

    klog("Temp copy map addr %x\n", v_map_addr);
    
    memcpy((void *)v_map_addr, (void *)src_virt, PAGE_SIZE); 
    knmunmap_contiguous(kern_buddy, pd, v_map_addr, 1);
    return 0;
}

int clone_page_table(struct page_dir *pd, pte_t pde, page_table_t *new_pt_ptr,
                     uintptr_t *new_pt_phys_addr) {

    page_table_t new_pt;
    page_table_t pt = pd->page_tables_virt[pde];
    
    if (!new_pt_phys_addr || !new_pt_ptr) {
        return EINVAL;
    }
    
    *new_pt_ptr = slab_alloc_from_cache(slab_cache);
    *new_pt_phys_addr = ptr_to_phys_addr(pd, *new_pt_ptr);
    new_pt = *new_pt_ptr;

    if (!new_pt) {
        return EINVAL;
    }

    for (unsigned int i = 0; i < PT_SIZE; i++) {
        uint16_t flags = get_tab_flags(pt[i]);

        if (pt[i] & USER && pt[i] & PRESENT) {
            if (find_alloc_frame(&new_pt[i], flags)) {
                klog_error("Frame allocation failed\n");
                return ENOMEM;
            }

            klog("Copy page data %x\n", new_pt[i]);
            
            uintptr_t paddr_new_pt = get_tab_pure_addr(new_pt[i]);
            if (copy_page(pd, paddr_new_pt, get_virt_addr(pde, i))) {
                return 1;
            }
        } else {
            new_pt[i] = pt[i];
        }
    }

    return 0;
}

void free_pd(struct page_dir *pd) {
    if (!pd) {
        return;
    }
    
    slab_free(slab_cache, pd->page_tables);
    slab_free(slab_cache, pd->page_tables_virt);
    kfree(pd);
}

int clone_page_dir(struct page_dir *restrict pd,
                   struct page_dir *restrict new_pd, int r) {
    uintptr_t ptables_phys;

    if (!new_pd) {
        return EINVAL;
    }

    new_pd->page_tables = kmalloc_align(PAGE_SIZE, PAGE_SIZE);
    /* new_pd->page_tables = slab_alloc_from_cache(slab_cache); */
    ptables_phys = ptr_to_phys_addr(pd, new_pd->page_tables);
    new_pd->page_tables_virt = kmalloc(PAGE_SIZE);
    /* new_pd->page_tables_virt = slab_alloc_from_198cache(slab_cache); */
    /* memset(new_pd->page_tables_virt, 0, PAGE_SIZE); */

    /* klog("Slab allocaed page tables\n"); */
    
    if (!new_pd->page_tables || !new_pd->page_tables_virt) {
        return ENOMEM;
    }

    new_pd->pd_phys_addr = ptables_phys;

    for (unsigned int i = 0; i < PT_SIZE; i++) {

        if (pd->page_tables[i] & ACCESSED) {
            pd->page_tables[i] &= ~ACCESSED;
        }


        if (pd->page_tables[i] & USER && tab_present(pd->page_tables[i])) {
            int ret;
            page_table_t new_pt;
            uintptr_t new_pt_paddr;

            /* klog("Clone the page table\n"); */

            ret = clone_page_table(pd, i, &new_pt, &new_pt_paddr);

            if (ret) {
                return ret;
            }
            
            new_pd->page_tables[i] = new_pt_paddr;
            new_pd->page_tables_virt[i] = new_pt;
        } else {
            new_pd->page_tables_virt[i] = pd->page_tables_virt[i];
            new_pd->page_tables[i] = pd->page_tables[i];

            /* klog("cl Pddd: %x\n", pd->page_tables_virt[768][1]); */
        }
    }

    return 0;
}

int clone_cur_page_dir(struct page_dir *new_pd) {
    return clone_page_dir(cur_pd, new_pd, 0);
}

int switch_page_dir_asm(uintptr_t pd);

int switch_page_dir(struct page_dir *pd) {
    if (cur_pd == pd) {
        klog_warn("Page directory already selected\n");
        return 0;
    }
    klog_warn("Switched page dir\n");
    
    cur_pd = pd;
    switch_page_dir_asm(pd->pd_phys_addr);
    /* flush_tlb(); */
    return 0;
}

uintptr_t *get_pd_page(struct page_dir *pd, uint16_t pde, uint16_t pte) {
    return &(((uintptr_t *)pd->page_tables_virt[pde])[pte]);
}

int pt_present(struct page_dir *pd, uint16_t pde) {
    return tab_present(pd->page_tables[pde]);
}

int page_present(struct page_dir *pd, uint16_t pde, uint16_t pte) {
    return tab_present(*get_pd_page(pd, pde, pte));
}

int get_pde_pte(uintptr_t addr, uint16_t *pde_p, uint16_t *pte_p) {
    if (!pde_p || !pte_p) {
        return 1;
    }
    
    *pde_p = addr >> 22;
    *pte_p = (addr >> 12) & 0x3ff;
    return 0;
}

void unmap_page(struct page_dir *pd, pte_t pde, pte_t pte) {
    page_table_t pt = (uintptr_t *)pd->page_tables_virt[pde];
    pt[pte] &= ~PRESENT;
}

void unmap_pages(struct page_dir *pd, pte_t pde, pte_t pte, size_t npages) {
    int overflow = 0;
    for (unsigned int i = 0; i < npages; i ++) {
        if (pte + overflow + i >= PT_SIZE) {
            overflow -= PT_SIZE;
            pde++;
        }

        if (pde >= PT_SIZE) {
            break;
        }
        
        unmap_page(pd, pde, overflow + pte + i);
    }
}

int non_present_page_hanler(uint16_t pde, uint16_t pte) {
    if (!pt_present(cur_pd, pde)) {
        /* klog_warn("Page table not present\n"); */
        int ret;
        page_table_t pt;

        ret = map_alloc_pt(cur_pd, &pt, pde, R_W | PRESENT);
        if (ret) {
            return ret;
        }
        
        ret = find_alloc_frame(&pt[pte], R_W | PRESENT);
        if (ret) {
            return ret;
        }

        flush_page(get_virt_addr(pde, pte));
    } else if (!page_present(cur_pd, pde, pte)) {
        /* klog_warn("Page not present\n"); */
        uintptr_t *pt_entry = get_pd_page(cur_pd, pde, pte);
        find_alloc_frame(pt_entry, R_W | PRESENT);
    }

    return 1;
}

void page_fault(struct isr_handler_args *args) {
    uintptr_t fault_addr;
    uint16_t pde;
    uint16_t pte;

    __asm__ volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    if (fault_addr != last_fault_addr) {
        klog_warn("Page fault at addr: %x\n", fault_addr); 
    }

    last_fault_addr = fault_addr;
    get_pde_pte(fault_addr, &pde, &pte);

    /* fb_print_hex(pte); */

    if (args->error ^ PRESENT) {
        non_present_page_hanler(pde, pte);
    } else if (args->error ^ R_W) {

    }
        
    /* fb_print_num(args.error); */
}

