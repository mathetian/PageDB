//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _HASH_FUNCTION_H
#define _HASH_FUNCTION_H

#include <stdint.h>
#include <Slice.h>

uint32_t MurmurHash3(const Slice & key);

/**Two samples to describe how to use it**/
/**Bad Hashing Function for Integer-like string**/
inline uint32_t defaultHashFunc(const Slice & key)
{
	/**Sorry, assume the length is equal to 32**/
    uint64_t value = 0x238F13AF;
    value *= key.returnAsInt();

    for(int index = 0; index < key.size(); index++)
        value = (value + (key[index] << (index*5 % 24))) & 0x7FFFFFFF;

    return value;
}

#endif // _MURMURHASH3_H_
