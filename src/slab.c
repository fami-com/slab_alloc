#include "slab.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem/pimem.h"
#include "utils.h"

#pragma GCC poison fork

void cache_init(struct Cache *cache, size_t object_size) {
    cache->next = cache->prev = NULL;
    cache->empty = cache->partial = cache->full = NULL;
    cache->object_count = 0;
    cache->object_size = object_size;
}

struct Cache *cache_create(struct Cache *cache, size_t object_size) {
    void *mem = slab_alloc(cache, CACHE_SIZE);
    if (mem == NULL) return NULL;

    cache_init(mem, object_size);
    return mem;
}

struct Cache *cache_search(struct Cache *cache, size_t object_size, struct Cache **last_cache) {
    do {
        if (cache->object_size == object_size) return cache;
        *last_cache = cache;
        cache = cache->next;
    } while (cache != NULL);

    return NULL;
}

void alloc_init(struct Cache *cache) {
    cache_init(cache, CACHE_SIZE);
}

struct Slab *slab_create(struct Cache *cache, size_t object_size){
    errno = 0;
    size_t p_size = align_to_page(object_size);
    if(errno != 0) return NULL;

    void *mem = pialloc(p_size);
    if (mem == NULL) return NULL;

    return slab_init(cache, mem, object_size, 1);
}

struct Slab *slab_create_large(struct Cache *cache, size_t object_size) {
    errno = 0;
    size_t p_size = align_to_page(object_size * 8);
    if(errno != 0) return NULL;

    void *mem = pialloc(p_size);
    if (mem == NULL) return NULL;

    size_t page_span = p_size / get_page_size();

    return slab_init(cache, mem, object_size, page_span);
}

struct Slab *slab_init(struct Cache *cache, void *memory, size_t object_size, size_t page_span) {
    struct Cache *cache_cache = cache, *last_cache;
    cache = cache_search(cache, object_size, &last_cache);

    if (cache == NULL) {
        cache = cache_create(cache_cache, object_size);
        if (cache == NULL) return NULL;
        last_cache->next = cache;
        cache->prev = last_cache;
    }

    struct Slab *slab = memory;
    slab->next = slab->prev = NULL;
    slab->object_size = object_size;
    slab->cache = cache;
    slab->page_span = page_span;

    if (cache->empty == NULL) {
        cache->empty = slab;
    } else {
        slab->next = cache->empty;
        cache->empty->next = slab;
    }

    return slab;
}

void cache_destroy(struct Cache *global_cache, struct Cache *cache) {
    struct Slab *s = cache->empty;

    while (s != NULL) {
        struct Slab *s1 = s->next;
        slab_destroy(s);
        s = s1;
    }

    s = cache->partial;

    while (s != NULL) {
        struct Slab *s1 = s->next;
        slab_destroy(s);
        s = s1;
    }

    s = cache->full;

    while (s != NULL) {
        struct Slab *s1 = s->next;
        slab_destroy(s);
        s = s1;
    }

    slab_free(global_cache, cache);
}

void alloc_destroy(struct Cache *cache) {
    struct Cache *cache_cache = cache;
    struct Cache *cache1;
    do {
        cache1 = cache->next;
        cache_destroy(cache_cache, cache);
        cache = cache1;
    } while(cache1 != NULL);
}

void *slab_alloc(struct Cache *cache, size_t object_size) {
    struct Cache *cache_cache = cache, *last_cache;

    bool is_large = object_size >= get_page_size() / 8;

    if (is_large) {
        object_size += 2 * ALIGNMENT;
    }

    cache = cache_search(cache, object_size, &last_cache);
    if (cache == NULL) {
        cache = cache_create(cache_cache, object_size);
        if (cache == NULL) return NULL;
        last_cache->next = cache;
        cache->prev = last_cache;
    }

    int bit_idx = -1;
    size_t i = 0;
    struct Slab *search_slab;
    size_t offset;

    if (cache->partial != NULL) {
        search_slab = cache->partial;
        size_t bitfield_size = SLAB_BIT_FIELD_SIZE(search_slab);

        for (; i < bitfield_size; i++) {
            bit_idx = find_first_unset(search_slab->bitmasks[i]);
            if (bit_idx != -1) break;
        }
    } else {
        if (cache->empty == NULL) {
            struct Slab *t;

            if (is_large) {
                t = slab_create_large(cache, object_size);
            } else {
                t = slab_create(cache, object_size);
            }

            if (t == NULL) return NULL;
        }

        cache->partial = search_slab = cache->empty;
        cache->empty = NULL;
        bit_idx = 0;
    }

    search_slab->bitmasks[i] |= 1 << bit_idx;

    size_t object_count = slab_used(search_slab);

    size_t max_objs = SLAB_MAX_OBJECTS(search_slab);

    if(object_count == max_objs) {
        cache->partial = search_slab->next;
        search_slab->next = cache->full;
        if (cache->full != 0) cache->full->prev = search_slab;
        cache->full = search_slab;
    }

    cache->object_count += 1;

    offset = (i * 8 + bit_idx) * search_slab->object_size;

    uintptr_t dbgtmp = (uintptr_t)body_from_slab(search_slab);
    uintptr_t mem = dbgtmp + offset;

    if(is_large) {
        mem -= ALIGNMENT;
        if (mem % (2 * ALIGNMENT)) {
            mem -= ALIGNMENT;
        }
        union Object *obj = (union Object*)mem;
        obj->owner = search_slab;
        mem += sizeof(union Object);
        *(unsigned long long*)mem = 0xFEDCBA9876543210ULL;
        mem += sizeof(unsigned  long  long );
    }

    return (void*)mem;
}

void slab_free(struct Cache *cache, void *addr) {
    if (addr == NULL) return;
    struct Slab *slab = slab_from_payload(addr);
    void *body = body_from_slab(slab);

    size_t diff = ((char*)addr - (char*)body) / slab->object_size;

    size_t n = diff / 8;
    size_t bit_idx = diff % 8;

    slab->bitmasks[n] &= ~(1 << bit_idx);

    size_t object_count = slab_used(slab);

    cache = slab->cache;

    cache->object_count -= 1;

    size_t max_objs = SLAB_MAX_OBJECTS(slab);

    if(object_count == max_objs - 1) {
        if (slab->prev != NULL) slab->prev->next = slab->next;
        if (slab->next != NULL) slab->next->prev = slab->prev;
        slab->next = cache->partial;
        if (cache->partial != NULL) cache->partial->prev = slab;
        cache->partial = slab;
        cache->full = NULL;
    } else if (object_count == 0) {
        if (slab->prev != NULL) slab->prev->next = slab->next;
        if (slab->next != NULL) slab->next->prev = slab->prev;
        slab->next = cache->empty;
        if (cache->empty != NULL) cache->empty->prev = slab;
        cache->empty = slab;
        cache->partial = NULL;
    }
}

size_t slab_used(struct Slab *slab) {
    size_t ret = 0;
    size_t n = SLAB_BIT_FIELD_SIZE(slab);
    for(size_t i = 0; i < n; i++) {
        ret += popcnt(slab->bitmasks[i]);
    }

    return ret;
}

int slab_destroy(struct Slab *slab) {
    if (slab_used(slab) != 0) return -1;
    pifree(slab, slab->page_span * get_page_size());
    return 0;
}

size_t ptr_size(void *addr) {
    return slab_from_payload(addr)->object_size;
}

struct Slab *slab_from_payload(void *addr) {
    uintptr_t ptr = (uintptr_t)addr;
    size_t rem = ptr % (2 * ALIGNMENT);
    printf("%zX, %zu, %zu\n", ptr, ptr % ALIGNMENT, ptr % (2 * ALIGNMENT));
    if (rem == 0) {
        return (struct Slab *) align_to_page_downwards((uintptr_t) addr);
    } else {
        char *p = (char*)addr - ALIGNMENT;
        struct Slab *slab = ((union Object*)p)->owner;
        unsigned long long chk = *(unsigned long long*)(p + sizeof(union Object));
        assert(chk == 0xFEDCBA9876543210ULL && "Bad checksum");
        return slab;
    }
}
