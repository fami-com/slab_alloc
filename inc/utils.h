#ifndef ALLOC_UTILS_H
#define ALLOC_UTILS_H

#include <stdint.h>
#include <stddef.h>

int popcnt(uint8_t v);
int find_first_unset(uint8_t v);

void print_u8_bits(uint8_t v);

#endif //ALLOC_UTILS_H
