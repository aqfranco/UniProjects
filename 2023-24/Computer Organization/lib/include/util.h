#pragma once

// Only works if n is a power of 2
#if defined(__GNUC__)
    #define LOG2(n) (__builtin_popcount(((n)-1)))
#else
    #error "No popcount"
#endif


#define SHIFT_MASK(value, shift, mask) (((value) >> (shift)) & (mask))