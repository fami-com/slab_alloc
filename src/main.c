#include "test.h"
#include "allocator.h"
#include <stdlib.h>
#include <limits.h>
#include "allocator.h"

#define N 100000U

int main() {
    //test(32, N);
    test(512, N);
    test(USHRT_MAX, N);
    return 0;
}
