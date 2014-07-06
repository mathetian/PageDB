// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _EMPTY_CACHE_H
#define _EMPTY_CACHE_H

#include "Cache.h"

namespace customdb
{

class EmptyCache : public BaseCache
{
public:
    EmptyCache() { }

public:
    bool    put(const Slice & key, const Slice & value)
    {
        return true;
    }

    Slice  	get(const Slice & key)
    {
        return BaseCache::get(key);
    }

    bool    remove(const Slice & key)
    {
        return true;
    }

    void    clear() { }
};

};

#endif