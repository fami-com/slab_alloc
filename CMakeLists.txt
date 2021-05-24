cmake_minimum_required(VERSION 3.19)
project(alloc C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -O0 -Wall -Wpedantic -Wextra -Wpointer-arith")

set(allocator_SOURCES inc/allocator.h inc/mem/pimem.h inc/test.h inc/slab.h src/allocator.c src/main.c src/test.c src/slab.c src/mem/pimem.c src/utils.c inc/utils.h)

include_directories(inc)
link_libraries(m)

if(WIN32)
    list(APPEND allocator_SOURCES src/mem/winmem.c)
elseif(UNIX)
    list(APPEND allocator_SOURCES src/mem/unixmem.c)
endif()

add_executable(alloc ${allocator_SOURCES})
