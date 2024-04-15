#pragma once

#include <stdint.h>
#include "private/Cache.h"

#include <util.h>
#include <cpu.h>

#define INCLUDE_L1_CACHE_INFO
// ================== L1 ==================
#ifdef INCLUDE_L1_CACHE_INFO

    #ifndef L1_ASSOCIATIVITY
        #define L1_ASSOCIATIVITY 1
    #endif
    #define L1_NUM_CACHE_LINE_SETS  (L1_SIZE / BLOCK_SIZE / L1_ASSOCIATIVITY)
    #define L1_NUM_CACHE_LINES      (L1_SIZE / BLOCK_SIZE)

    #define L1_ADDRESS_OFFSET_SHIFT 0
    #define L1_ADDRESS_OFFSET_BITS LOG2(BLOCK_SIZE)
    #define L1_ADDRESS_OFFSET_MASK (BLOCK_SIZE - 1u)

    #define L1_ADDRESS_INDEX_SHIFT L1_ADDRESS_OFFSET_BITS
    #define L1_ADDRESS_INDEX_BITS LOG2(L1_NUM_CACHE_LINE_SETS)
    #define L1_ADDRESS_INDEX_MASK (L1_NUM_CACHE_LINE_SETS - 1u)

    #define L1_ADDRESS_TAG_SHIFT (L1_ADDRESS_INDEX_BITS + L1_ADDRESS_INDEX_SHIFT)

    #define GET_L1_OFFSET(addr) SHIFT_MASK((addr), L1_ADDRESS_OFFSET_SHIFT, L1_ADDRESS_OFFSET_MASK)
    #define GET_L1_INDEX(addr) SHIFT_MASK((addr), L1_ADDRESS_INDEX_SHIFT, L1_ADDRESS_INDEX_MASK)
    #define GET_L1_TAG(addr) ((addr) >> L1_ADDRESS_TAG_SHIFT)

    #define L1_NEXT_CACHE_LEVEL_FUNC accessDRAM

    typedef struct {
        uint8_t Present : 1;
        uint8_t Dirty : 1;
        uint16_t padd;
        uint32_t tag;
    } L1CacheLine;

    typedef struct {
        L1CacheLine way[L1_ASSOCIATIVITY];
        uint8_t lru[L1_ASSOCIATIVITY];
        uint8_t lru_size;
    } L1Set;

    typedef struct {
        L1Set sets[L1_NUM_CACHE_LINE_SETS];
    } CACHE_ALIGNED L1Cache;

#endif
// ================== L2 ==================
#ifdef INCLUDE_L2_CACHE_INFO

    #ifndef L2_ASSOCIATIVITY
        #define L2_ASSOCIATIVITY 1
    #endif
    #define L2_NUM_CACHE_LINE_SETS  (L2_SIZE / BLOCK_SIZE / L2_ASSOCIATIVITY)
    #define L2_NUM_CACHE_LINES      (L2_SIZE / BLOCK_SIZE)

    #define L2_ADDRESS_OFFSET_SHIFT 0
    #define L2_ADDRESS_OFFSET_BITS LOG2(BLOCK_SIZE)
    #define L2_ADDRESS_OFFSET_MASK (BLOCK_SIZE - 1u)

    #define L2_ADDRESS_INDEX_SHIFT L2_ADDRESS_OFFSET_BITS
    #define L2_ADDRESS_INDEX_BITS LOG2(L2_NUM_CACHE_LINE_SETS)
    #define L2_ADDRESS_INDEX_MASK (L2_NUM_CACHE_LINE_SETS - 1u)

    #define L2_ADDRESS_TAG_SHIFT (L2_ADDRESS_INDEX_BITS + L2_ADDRESS_INDEX_SHIFT)
    
    #define GET_L2_OFFSET(addr) SHIFT_MASK((addr), L2_ADDRESS_OFFSET_SHIFT, L2_ADDRESS_OFFSET_MASK)
    #define GET_L2_INDEX(addr) SHIFT_MASK((addr), L2_ADDRESS_INDEX_SHIFT, L2_ADDRESS_INDEX_MASK)
    #define GET_L2_TAG(addr) ((addr) >> L2_ADDRESS_TAG_SHIFT)

    #ifdef L1_NEXT_CACHE_LEVEL_FUNC
        #undef L1_NEXT_CACHE_LEVEL_FUNC
    #endif
    #define L1_NEXT_CACHE_LEVEL_FUNC accessL2

    #define L2_NEXT_CACHE_LEVEL_FUNC accessDRAM

    typedef struct {
        uint8_t Present : 1;
        uint8_t Dirty : 1;
        uint16_t padd;
        uint32_t tag;
    } L2CacheLine;

    typedef struct {
        L2CacheLine way[L2_ASSOCIATIVITY];
        uint8_t lru[L2_ASSOCIATIVITY];
        uint8_t lru_size;
    } L2Set;

    typedef struct {
        L2Set sets[L2_NUM_CACHE_LINE_SETS];
    } CACHE_ALIGNED L2Cache;

#endif
// ========================================
