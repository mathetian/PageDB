// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _HASH_FUNCTION_H
#define _HASH_FUNCTION_H

#include "../include/Slice.h"

/**
** There are many hash-functions can be used in producation environment
**
** However, MurmurHash is the first choice in most situation as it has a high speed
** MurmurHash can be found in `Redis` and `leveldb`(leveldb uses a variant of MurmurHash)
** The detail information can be found in <a href="http://en.wikipedia.org/wiki/MurmurHash">wikipedia</a>.
**/
namespace utils
{

uint32_t MurmurHash3(const Slice & key);
uint32_t DefaultHashFunc(const Slice & key);

}
#endif
