#include <windows.h>

#include "pimem.h"
#include "memoryapi.h"
#include "arena.h"

static size_t page_size;

size_t get_page_size() {
    if (!page_size) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
    }

    return page_size;
}

void *pialloc(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void pifree(void *ptr, size_t size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void piadvise(void *ptr, size_t size) {
    VirtualAlloc(ptr, size, MEM_RESET, PAGE_READWRITE);
}
