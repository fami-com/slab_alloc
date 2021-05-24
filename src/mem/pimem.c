#include "mem/pimem.h"

size_t align_to(size_t size, size_t alignment) {
    size_t ret_size = size;
    size_t rem = size % alignment;

    if (rem != 0) {
        ret_size += alignment - rem;
        if (ret_size < size) {
            errno = ERANGE;
            return 0;
        }
    }

    return ret_size;
}

size_t align_to_downwards(uintptr_t size, size_t alignment) {
    size_t rem = size % alignment;

    if (rem != 0) {
        size -= rem;
    }

    return size;
}
