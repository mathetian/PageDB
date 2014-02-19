// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#include "HashFunction.h"

namespace customdb
{

#define ROTL32(x,y) rotl32(x,y)
#define ROTL64(x,y) rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

#define getblock(x, i) (x[i])

static uint32_t rotl32 ( uint32_t x, int8_t r )
{
    return (x << r) | (x >> (32 - r));
}

static uint64_t rotl64 ( uint64_t x, int8_t r )
{
    return (x << r) | (x >> (64 - r));
}

uint32_t fmix32(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

uint64_t fmix64(uint64_t k)
{
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;

    return k;
}

uint32_t MurmurHash3(const Slice & key)
{
    /**Maybe changed into your favorite**/
    uint32_t seed = 0x238F13AF;
    const void * key1 = key.c_str();
    const uint8_t * data = (const uint8_t*)key1;
    int len = key.size();
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;

    int i;

    const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

    for(i = -nblocks; i; i++)
    {
        uint32_t k1 = getblock(blocks,i);

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

    uint32_t k1 = 0;

    switch(len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;
        h1 ^= k1;
    }

    h1 ^= len;

    h1 = fmix32(h1);

    return h1;
}

uint32_t defaultHashFunc(const Slice & key)
{
    uint64_t value = 0x238F13AF | key.size();

    for(int index = 0; index < key.size(); index++)
        value = (value + (key[index] << (index*5 % 24))) & 0x7FFFFFFF;

    return value;
}

};