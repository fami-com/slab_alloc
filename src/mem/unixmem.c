#include <sys/mman.h>
#include <unistd.h>

#include "mem/pimem.h"
//#include "arena.h"

static size_t page_size;

void *pialloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    return ptr;
}

void pifree(void *ptr, size_t size) {
    munmap(ptr, size);
}

void piadvise(void *ptr, size_t size) {
    madvise(ptr, size, MADV_DONTNEED);
}

size_t get_page_size() {
    if (!page_size) {
        page_size = sysconf(_SC_PAGESIZE);
    }
    return page_size;
}


