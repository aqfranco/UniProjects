#include "SimpleCache.h"

L1Cache CacheL1;
CACHE_ALIGNED uint8_t L1CacheBuff[L1_SIZE];

#ifdef INCLUDE_L2_CACHE_INFO
L2Cache CacheL2;
CACHE_ALIGNED uint8_t L2CacheBuff[L2_SIZE];
#endif

uint8_t DRAM[DRAM_SIZE];

uint32_t time;

void debugPrintf(const char *fmt, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

/*********************** L1 cache *************************/

void initCache() {
    discardCache();

    debugPrintf("\nConfig:\n");
    debugPrintf("L1 size: %uB %u-way associative\nLine size of %u bytes.\n", L1_SIZE, L1_ASSOCIATIVITY, BLOCK_SIZE);
    debugPrintf("Set offset mask: 0x%X\n", (BLOCK_SIZE - 1));
    debugPrintf("index rshift(%u) + mask(bitcnt: %u mask:0x%X)\n", L1_ADDRESS_INDEX_SHIFT, L1_ADDRESS_INDEX_BITS, L1_ADDRESS_INDEX_MASK);
    debugPrintf("tag shift amount: %u\n\n\n", L1_ADDRESS_TAG_SHIFT);
}

void discardCache() {
    memset(CacheL1.sets, 0, sizeof(CacheL1.sets));
#ifdef INCLUDE_L2_CACHE_INFO
    memset(CacheL2.sets, 0, sizeof(CacheL2.sets));
#endif
}


static inline
uint8_t *indexL1(uint32_t index, uint32_t set) {
    return &L1CacheBuff[(index * L1_ASSOCIATIVITY + set) * BLOCK_SIZE];
}

#ifdef INCLUDE_L2_CACHE_INFO
static inline
uint8_t *indexL2(uint32_t index, uint32_t set) {
    return &L2CacheBuff[(index * L2_ASSOCIATIVITY + set) * BLOCK_SIZE];
}

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t index = GET_L2_INDEX(address);
    uint32_t tag = GET_L2_TAG(address);
    uint32_t offset = GET_L2_OFFSET(address);
    assert(offset == 0);

    debugPrintf("L2 tag: %u index: %u for address %u\n", tag, index, address);

    L2Set *set = &CacheL2.sets[index];
    uint8_t *cacheLineData = NULL;

    // Find set index and/or empty slot
    int32_t empty_set_index = -1;
    int32_t set_index = -1;
    for (int32_t i = 0; i < L2_ASSOCIATIVITY; i++) {

        if (!set->way[i].Present) {
            empty_set_index = i;
            continue;
        }

        if (set->way[i].tag != tag) {
            continue;
        }

        // Cache hit
        set_index = i;
        cacheLineData = indexL2(index, i);
        goto accessCache;
    }

    // Cache miss
    debugPrintf("L2 empty_set_index %i\n", empty_set_index);

    if (empty_set_index == -1) {
        assert(set->lru_size == L2_ASSOCIATIVITY);

        // Evict last lru element
        uint32_t replace_set_index = set->lru[L2_ASSOCIATIVITY - 1];

        if (set->way[replace_set_index].Dirty) {
            uint32_t writeback_addr =
                (set->way[replace_set_index].tag << L2_ADDRESS_TAG_SHIFT) | (index << L2_ADDRESS_INDEX_SHIFT);

            L2_NEXT_CACHE_LEVEL_FUNC(writeback_addr, indexL2(index, replace_set_index), MODE_WRITE);
            debugPrintf("L2 Evict + Writeback: %X\n", writeback_addr);
        }

        set->lru_size--; // Remove the last lru element
        set->way[replace_set_index].Present = 0;
        empty_set_index = replace_set_index;
    }

    cacheLineData = indexL2(index, empty_set_index);

    uint32_t cacheLineMemAddress = address & ~L2_ADDRESS_OFFSET_MASK;
    L2_NEXT_CACHE_LEVEL_FUNC(cacheLineMemAddress, cacheLineData, MODE_READ); // get new block from DRAM
    set->way[empty_set_index].Present = 1;
    set->way[empty_set_index].tag = tag;
    set->way[empty_set_index].Dirty = 0;

    set_index = empty_set_index;

accessCache:
    assert(set_index >= 0);

    if (mode == MODE_READ) {
        memcpy(data, cacheLineData, WORD_SIZE);
        time += L2_READ_TIME;
    } else if (mode == MODE_WRITE) {
        memcpy(cacheLineData, data, WORD_SIZE);
        set->way[set_index].Dirty = 1;
        time += L2_WRITE_TIME;
    }

    for (uint32_t lru_index = 0; lru_index < set->lru_size; lru_index++) {
        if (set->lru[lru_index] == set_index) {
            set->lru[lru_index] = set->lru[set->lru_size - 1];
            set->lru_size--;
            break;
        }
    }

    // push all elements 1 back
    memmove(&set->lru[1], &set->lru[0], ((L2_ASSOCIATIVITY - 1) * (sizeof(set->lru[0]))));
    set->lru[0] = set_index;
    set->lru_size++;
}

#endif

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t index = GET_L1_INDEX(address);
    uint32_t tag = GET_L1_TAG(address);
    uint32_t offset = GET_L1_OFFSET(address);
    assert((offset % 4) == 0);

    debugPrintf("L1 tag: %u index: %u for address %u\n", tag, index, address);

    L1Set *set = &CacheL1.sets[index];
    uint8_t *cacheLineData = NULL;

    // Find set index and/or empty slot
    int32_t empty_set_index = -1;
    int32_t set_index = -1;
    for (int32_t i = 0; i < L1_ASSOCIATIVITY; i++) {

        if (!set->way[i].Present) {
            empty_set_index = i;
            continue;
        }

        if (set->way[i].tag != tag) {
            continue;
        }

        // Cache hit
        set_index = i;
        cacheLineData = indexL1(index, i);
        goto accessCache;
    }

    // Cache miss
    debugPrintf("L1 empty_set_index %i\n", empty_set_index);

    if (empty_set_index == -1) {
        assert(set->lru_size == L1_ASSOCIATIVITY);

        // Evict last lru element
        uint32_t replace_set_index = set->lru[L1_ASSOCIATIVITY - 1];

        if (set->way[replace_set_index].Dirty) {
            uint32_t writeback_addr =
                (set->way[replace_set_index].tag << L1_ADDRESS_TAG_SHIFT) | (index << L1_ADDRESS_INDEX_SHIFT);

            L1_NEXT_CACHE_LEVEL_FUNC(writeback_addr, indexL1(index, replace_set_index), MODE_WRITE);
            debugPrintf("L1 Evict + Writeback: %X\n", writeback_addr);
        }

        set->lru_size--; // Remove the last lru element
        set->way[replace_set_index].Present = 0;
        empty_set_index = replace_set_index;
    }

    cacheLineData = indexL1(index, empty_set_index);

    uint32_t cacheLineMemAddress = address & ~L1_ADDRESS_OFFSET_MASK;
    L1_NEXT_CACHE_LEVEL_FUNC(cacheLineMemAddress, cacheLineData, MODE_READ); // get new block from DRAM
    set->way[empty_set_index].Present = 1;
    set->way[empty_set_index].tag = tag;
    set->way[empty_set_index].Dirty = 0;

    set_index = empty_set_index;

accessCache:
    assert(set_index >= 0);

    if (mode == MODE_READ) {
        memcpy(data, cacheLineData + offset, WORD_SIZE);
        time += L1_READ_TIME;
    } else if (mode == MODE_WRITE) {
        memcpy(cacheLineData + offset, data, WORD_SIZE);
        set->way[set_index].Dirty = 1;
        time += L1_WRITE_TIME;
    }

    for (uint32_t lru_index = 0; lru_index < set->lru_size; lru_index++) {
        if (set->lru[lru_index] == set_index) {
            set->lru[lru_index] = set->lru[set->lru_size - 1];
            set->lru_size--;
            break;
        }
    }

    // push all elements 1 back
    memmove(&set->lru[1], &set->lru[0], ((L1_ASSOCIATIVITY - 1) * (sizeof(set->lru[0]))));
    set->lru[0] = set_index;
    set->lru_size++;
}

void read(uint32_t address, uint8_t *data) {
    assert(address < DRAM_SIZE);
    debugPrintf("[read] at 0x%X\n", address);
    accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
    assert(address < DRAM_SIZE);
    debugPrintf("[write] to 0x%X value %u\n", address, *(uint32_t *)data);
    accessL1(address, data, MODE_WRITE);
}

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************    RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

    if (address >= DRAM_SIZE - WORD_SIZE + 1)
        exit(-1);

    if (mode == MODE_READ) {
        memcpy(data, &DRAM[address], BLOCK_SIZE);
        time += DRAM_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(&DRAM[address], data, BLOCK_SIZE);
        time += DRAM_WRITE_TIME;
    }
}