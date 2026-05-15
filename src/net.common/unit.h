#pragma once
#include <stdint.h>

#define GB 1024LL * 1024 * 1024
#define MB 1024 * 1024
#define KB 1024

template <size_t size>
struct alignas(64) memory_block 
{
    uint8_t data[size];
};
using block64 = memory_block<64>;
using block128 = memory_block<128>;
using block256 = memory_block<256>;
using block512 = memory_block<512>;
using block1024 = memory_block<1024>;
using block2048 = memory_block<2048>;
using block4096 = memory_block<4096>;