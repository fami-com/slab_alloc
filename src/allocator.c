#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "allocator.h"
#include "mem/pimem.h"
#include "slab.h"
#include "utils.h"

int is_alloc_initted = 0;
struct Cache cache;

#define min(a,b) ((a)<(b)?(a):(b))

void *mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    errno = 0;

    size = align(size);

    if (errno == ERANGE) {
        errno = ENOMEM;
        return NULL;
    }

    if(!is_alloc_initted) {
        alloc_init(&cache);
        is_alloc_initted = 1;
    }

    void *mem = slab_alloc(&cache, size);
    return mem;
}

void mem_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    slab_free(&cache, ptr);
}

void *mem_realloc(void *ptr, size_t new_size) {
    errno = 0;
    new_size = align(new_size);

    if (errno == ERANGE) {
        return NULL;
    }

    if (ptr == NULL) {
        return mem_alloc(new_size);
    }

    size_t old_size = ptr_size(ptr);
    if (old_size == new_size) return ptr;

    void *new_ptr = mem_alloc(new_size);

    memcpy(new_ptr, ptr, min(old_size, new_size));

    mem_free(ptr);

    return new_ptr;
}

/*void *calloc(size_t num, size_t size) {
    void *mem = mem_alloc(num * size);
    if (mem == NULL) return NULL;
    memset(mem, 0, num * size);
    return mem;
}*/

void mem_dump() {
    puts("--- START MEMORY DUMP ---");

    struct Cache *c = &cache;
    size_t no = 1;
    do {
        printf("Dumping cache #%lu with objects of size %lu with %lu objects, prev: %p, next: %p\n", no, c->object_size, c->object_count, (void*)c->prev, (void*)c->next);
        no++;

        if(c->empty == NULL) {
            puts("Cache has no empty slabs");
        } else {
            puts("Cache has empty slabs");
        }

        if(c->partial == NULL) {
            puts("Cache has no partial slabs");
        } else {
            puts("Cache has partial slabs with the following bitmasks:");
            struct Slab *s = c->partial;
            int idx = 1;
            do {
                printf("Slab #%d:\n", idx);

                size_t bit_size = SLAB_BIT_FIELD_SIZE(s);
                for (size_t i = 0, n = 1; i < bit_size; i++, n++) {
                    print_u8_bits(s->bitmasks[i]);
                    if (n == 8) {
                        putchar('\n');
                        n = 0;
                    } else {
                        putchar(' ');
                    }
                }
                s = s->next;
                idx++;
            } while (s != NULL);
        }

        if(c->full == NULL) {
            puts("Cache has no full slabs");
        } else {
            puts("Cache has full slabs");
        }

        c = c->next;
    } while (c != NULL);

    puts("--- FINISH MEMORY DUMP ---");
}
