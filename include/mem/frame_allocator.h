#include <stdint.h>

void frame_alloc_init(void);

uintptr_t alloc_frame(uintptr_t addr, unsigned int flags);

uintptr_t find_alloc_frame(unsigned int flags);

uintptr_t alloc_pt(unsigned int pde, unsigned int pte, uint16_t flags);

int dealloc_frame(uintptr_t addr);
