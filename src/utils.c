#include <stdio.h>
#include <errno.h>

#include "utils.h"
#include "mem/pimem.h"

int popcnt(uint8_t v) {
    int cnt = 0;
    for (; v; cnt++) {
        v &= v - 1;
    }

    return cnt;
}

int find_first_unset(uint8_t v) {
    for(int pos = 0; pos < 8; pos++, v >>= 1) {
        if ((v & 1) == 0) return pos;
    }

    return -1;
}

void print_u8_bits(uint8_t v) {
    for (int i = 7; 0 <= i; i--) {
        printf("%c", (v & (1 << i)) ? '1' : '0');
    }
}

/*
size_t power_ceil(size_t v) {
    if (v <= 1) return 1;

    size_t power = 2;
    v--;
    while (v >>= 1) power <<= 1;

    return power;
}

size_t power_floor(size_t v) {
    size_t power = 1;
    while (v >>= 1) power <<= 1;
    return power;
}
*/
