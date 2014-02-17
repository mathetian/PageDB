// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _HASH_FUNCTION_H
#define _HASH_FUNCTION_H

#include <stdint.h>

#include "Slice.h"

namespace customdb{

uint32_t MurmurHash3(const Slice & key);
/**Two samples to describe how to use it**/
/**Bad Hashing Function for Integer-like string**/
uint32_t DefaultHashFunc(const Slice & key);

}
#endif
