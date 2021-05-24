#ifndef SLAB_ALLOC_SLAB_H
#define SLAB_ALLOC_SLAB_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "mem/pimem.h"

#define CACHE_SIZE (align(sizeof(struct Cache)))
#define SLAB_BIT_FIELD_SIZE_OF(n, k) ((((n) * get_page_size() - sizeof(struct Slab)) + 8 * (k)) / (8 * (k) + 1))
#define SLAB_MAX_OBJECTS_OF(n, k) (((n) * get_page_size() - sizeof(struct Slab) - SLAB_BIT_FIELD_SIZE_OF(n, k)) / (k))
#define SLAB_SIZE(s) (align(sizeof(struct Slab) + SLAB_BIT_FIELD_SIZE_OF((s)->page_span, (s)->object_size)))
#define SLAB_MAX_OBJECTS(s) (SLAB_MAX_OBJECTS_OF((s)->page_span, (s)->object_size))
#define SLAB_BIT_FIELD_SIZE(s) (SLAB_BIT_FIELD_SIZE_OF((s)->page_span, (s)->object_size))

struct Slab {
    struct Slab *prev, *next;
    struct Cache *cache;
    size_t page_span;
    size_t object_size;
    uint8_t bitmasks[];
};

struct Cache {
    size_t object_size;
    size_t object_count;
    struct Cache *prev, *next;
    struct Slab *empty, *partial, *full;
};

union Object {
    struct Slab *owner;
    unsigned char data[sizeof(struct Slab *)];
};

struct Slab *slab_from_payload(void *addr);

static inline void *body_from_slab(struct Slab *slab){
    return (char*)slab + SLAB_SIZE(slab);
}

void alloc_init(struct Cache *cache);
void alloc_destroy(struct Cache *cache);

struct Cache *cache_create(struct Cache *cache, size_t object_size);
void cache_init(struct Cache *cache, size_t object_size);
struct Cache *cache_search(struct Cache *cache, size_t object_size, struct Cache **last_cache);
void cache_destroy(struct Cache *global_cache, struct Cache *cache);

size_t slab_used(struct Slab *slab);

struct Slab *slab_init(struct Cache *cache, void *memory, size_t object_size, size_t page_span);
struct Slab *slab_create(struct Cache *cache, size_t object_size);
struct Slab *slab_create_large(struct Cache *cache, size_t object_size);
int slab_destroy(struct Slab *slab);

void *slab_alloc(struct Cache *cache, size_t object_size);
void slab_free(struct Cache *cache, void *addr);

size_t ptr_size(void *addr);

#endif //SLAB_ALLOC_SLAB_H
