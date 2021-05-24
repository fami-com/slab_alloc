#ifndef ALLOC_PIMEM_H
#define ALLOC_PIMEM_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

void *pialloc(size_t size);

void pifree(void *ptr, size_t size);

void piadvise(void *ptr, size_t size);

size_t get_page_size();

#define ALIGNMENT _Alignof(max_align_t)

size_t align_to(size_t size, size_t alignment);
size_t align_to_downwards(uintptr_t size, size_t alignment);

static inline size_t align_to_page(size_t size) {
    return align_to(size, get_page_size());
}

static inline size_t align_to_page_downwards(uintptr_t size) {
    return align_to_downwards(size, get_page_size());
}

static inline size_t align(size_t size) {
    return align_to(size, 2 * ALIGNMENT);
}

static inline size_t align1(size_t size) {
    return align_to(size, ALIGNMENT);
}

#endif //ALLOC_PIMEM_H
