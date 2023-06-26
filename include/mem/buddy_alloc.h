#include <stdint.h>
#include <stdlib.h>

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes, uint16_t flags);

int buddy_alloc_frame(uintptr_t *addr, uint16_t flags);
