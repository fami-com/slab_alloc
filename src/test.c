#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "mem/pimem.h"
#include "test.h"
#include "allocator.h"

#define min(a,b) ((a)<(b)?(a):(b))

/*
#undef mem_free
#undef mem_alloc
#undef mem_realloc

#define mem_free free
#define mem_alloc malloc
#define mem_realloc realloc
*/

void fill_rand(void *ptr, size_t size) {
    unsigned char *cptr = ptr;
    for (size_t i = 0; i < size; i++) {
        *cptr++ = rand() % 256;
    }
}

unsigned int get_checksum(void *ptr, size_t size) {
    unsigned sum = 0;
    unsigned char *addr = ptr;

    for (size_t i = 0; i < size; i++) {
        unsigned char v = *(addr + i);
        sum += v;
    }

    return sum;
}

void test(size_t max_size, unsigned N) {
    time_t rand_seed = time(NULL);
    srand(1);
    struct OperationResult results[ARRAY_SIZE] = {{.addr = NULL, .size = 0, .checksum = 0}};
    printf("Starting test with max size %ld, seed %lu:\n", max_size, rand_seed);

    for (unsigned i = 0; i < N; i++) {
        // 0 - ALLOC
        // 1 - mem_realloc
        // 2 - mem_free
        unsigned char action = rand() % 3;
        size_t size = rand() % max_size + 1;
        unsigned rand_index = rand() % ARRAY_SIZE;
        struct OperationResult result;

        if(i % 1000 == 1) {
            //mem_dump();
        }

        switch (action) {
            case 0: { // ALLOC
                void *ptr = mem_alloc(size);
                if (ptr != NULL) {
                    fill_rand(ptr, size);
                    result = (struct OperationResult){
                            .addr = ptr,
                            .size = size,
                            .checksum = get_checksum(ptr, size),
                    };

                    //printf("At index: %u, ptr: %p, size: %lu, stored checksum: %u, calculated checksum: %u\n", rand_index, result.addr, result.size, result.checksum, get_checksum(result.addr, result.size));

                    assert(get_checksum(result.addr, result.size) == result.checksum && "bad checksum");
                    mem_free(results[rand_index].addr);
                    results[rand_index] = result;
                }
                break;
            }
            case 1: {
                result = results[rand_index];

                if (result.addr != NULL) {
                    assert(get_checksum(result.addr, result.size) == result.checksum && "bad checksum");
                }

                unsigned controll = get_checksum(result.addr, min(size, result.size));
                void *ptr = mem_realloc(result.addr, size);

                if (ptr != NULL) {
                    assert(get_checksum(ptr, min(size, result.size)) == controll && "bad checksum");
                    fill_rand(ptr, size);

                    //printf("At index: %u, ptr: %p, size: %lu, stored checksum: %u, calculated checksum: %u\n", rand_index, result.addr, result.size, result.checksum, get_checksum(result.addr, result.size));

                    results[rand_index] = (struct OperationResult){
                            .addr = ptr,
                            .size = size,
                            .checksum = get_checksum(ptr, size),
                    };
                }
                break;
            }
            case 2: {
                result = results[rand_index];

                if (result.addr != NULL) {
                    //printf("At index: %u, ptr: %p, size: %lu, stored checksum: %u, calculated checksum: %u\n", rand_index, result.addr, result.size, result.checksum, get_checksum(result.addr, result.size));
                    assert(get_checksum(result.addr, result.size) == result.checksum && "bad checksum");
                }

                mem_free(result.addr);
                results[rand_index] = (struct OperationResult){
                        .addr = NULL,
                        .size = 0,
                        .checksum = 0,
                };

                break;
            }
            default:
                break;
        }
    }

    for (unsigned i = 0; i < ARRAY_SIZE; i++) {
        assert(get_checksum(results[i].addr, results[i].size) == results[i].checksum && "bad checksum");
        mem_free(results[i].addr);
    }

    printf("Test with maximum size %ld finished successfully\n", max_size);
}
