#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

// Own headers
#include "cache.h"
#include <util.h>
#include <cpu.h>


/*********************** Cache *************************/

void initCache();
void discardCache();
void accessL1(uint32_t, uint8_t *, uint32_t);

/*********************** Timer *************************/

void resetTime();

uint32_t getTime();

/****************    RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);
