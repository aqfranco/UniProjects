#include "SimpleCache.h"
#include <stdbool.h>

#define STYLE_RESET "\033[0m"
#define STYLE_BOLD "\033[1m"
#define CL_RED   "\033[91m"
#define CL_GREEN "\033[92m"

#define TEST_FAILED(fmt) (STYLE_BOLD CL_RED "Test Failed \xE2\x9C\x96 " fmt STYLE_RESET)

void passed() {
    printf(STYLE_BOLD CL_GREEN "Test Passed \xE2\x9C\x94\n" STYLE_RESET);
}

#define NUM_BLOCKS (L1_SIZE / BLOCK_SIZE)

int main() {

    initCache();
    const uint32_t conflictStride = L1_NUM_CACHE_LINE_SETS * BLOCK_SIZE;

    { // Basic read/write test
        resetTime();
        discardCache();
        uint32_t write_val = -1;
        uint32_t read_val = 0;
        uint32_t address = 0x0;
        // Basic functionality as in the initial test
        write(address, (uint8_t *)(&write_val));
        uint32_t time1 = getTime();
        read(address, (uint8_t *)(&read_val));
        uint32_t time2 = getTime();

        uint32_t expect1 = L1_WRITE_TIME + DRAM_READ_TIME;
        uint32_t expect2 = expect1 + L1_READ_TIME;
        if (time1 != expect1) {
            printf(TEST_FAILED("Time: Got %u but expected %u.\n"), time1, expect1);
        } else if (time2 != expect2) {
            printf(TEST_FAILED("Time: Got %u but expected %u.\n"), time2, expect2);
        } else if (write_val != read_val) {
            printf(TEST_FAILED("Value: Got %u but expected %u.\n"), read_val, write_val);
        } else {
            passed();
        }
    }

    { // Test all memory
        resetTime();
        discardCache();
        bool failed = false;
        for (uint32_t i = 0; i < DRAM_SIZE; i += BLOCK_SIZE) {
            uint32_t val = i / BLOCK_SIZE;
            write(i, (uint8_t *)&val);
        }

        for (uint32_t i = 0; i < DRAM_SIZE; i += BLOCK_SIZE) {
            uint32_t val = i / BLOCK_SIZE;
            uint32_t read_val = 0;

            read(i, (uint8_t *)&read_val);

            if (val != read_val) {
                printf(TEST_FAILED("Got %u but expected %u.\n"), read_val, val);
                failed = true;
                break;
            }
        }
        if (!failed)
            passed();
    }

    { // Stress testing a single set
        resetTime();
        discardCache();
        bool failed = false;
        for (uint32_t i = 0; i < DRAM_SIZE / conflictStride; i += conflictStride) {
            uint32_t value = ~(i * 4u + i * 1053u + 17u); // Generate some pseudo random number
            write(i, (uint8_t *)&value);
        }
        for (uint32_t i = 0; i < DRAM_SIZE / conflictStride; i += conflictStride) {
            uint32_t expect = ~(i * 4u + i * 1053u + 17u);
            uint32_t value = 0;

            read(i, (uint8_t *)&value);

            if (value != expect) {
                printf(TEST_FAILED("Got %u but expected %u.\n"), value, expect);
                failed = true;
                break;
            }
        }
        if (!failed)
            passed();
    }

    { // Testing associativity time
        resetTime();
        discardCache();
        uint32_t testvalues[L1_ASSOCIATIVITY + 1];
        for (uint32_t i = 0; i < L1_ASSOCIATIVITY + 1; i++) {
            testvalues[i] = i * 2;
            write(i * (L1_NUM_CACHE_LINE_SETS * BLOCK_SIZE), (uint8_t *)(&testvalues[i]));
        }
        uint32_t expected = (L1_WRITE_TIME + DRAM_READ_TIME) * (L1_ASSOCIATIVITY + 1) + DRAM_WRITE_TIME;
        uint32_t time = getTime();
        if (expected == time) {
            passed();
        } else {
            printf(TEST_FAILED("Got %u but expected %u.\n"), time, expected);
        }
    }

    { // Testing associativity values
        resetTime();
        discardCache();
        bool failed = false;
        uint32_t testvalues[L1_ASSOCIATIVITY + 1];
        for (uint32_t i = 0; i < L1_ASSOCIATIVITY + 1; i++) {
            testvalues[i] = i * 2;
            write(i * (L1_NUM_CACHE_LINE_SETS * BLOCK_SIZE), (uint8_t *)(&testvalues[i]));
        }
        for (uint32_t i = 0; i < L1_ASSOCIATIVITY + 1; i++) {
            uint32_t temp;
            read(i * (L1_NUM_CACHE_LINE_SETS * BLOCK_SIZE), (uint8_t *)&temp);
            if (temp != testvalues[i]) {
                printf(TEST_FAILED("Got %u but expected %u.\n"), testvalues[i], temp);
                failed = true;
                break;
            }
        }
        if (!failed)
            passed();
    }

    return 0;
}