#include <stdint.h>
#include <stddef.h>

void buddy_test(size_t mem);

int buddy_alloc_init(size_t mem_limit);

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes, uint16_t flags);

int buddy_alloc_frame(uintptr_t *addr, uint16_t flags);

int buddy_alloc_at_addr(uintptr_t base, uintptr_t *addrs, size_t nframes,
                        uint16_t flags);
