#pragma once

#define THIS_CPU_BLOCK_SIZE 64

#if defined(__GNUC__)
    #define CACHE_ALIGNED __attribute__((aligned(THIS_CPU_BLOCK_SIZE)))
#else
    #define CACHE_ALIGNED
#endif