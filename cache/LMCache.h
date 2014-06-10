// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _LM_CACHE_H
#define _LM_CACHE_H

#include "BaseCache.h"

/**
** LMCache states that the cache memory should be controlled into a suitable size.
** Cache size is calcuated by the sum of sizes of values in cache system.
**/
namespace customdb
{

class LimitedMemoryCache : public BaseCache
{
public:
    /**
    ** cacheLimitInMB the default size of cache system
    **/
    LimitedMemoryCache(int cacheLimitInMB = defaultCacheSizeInMB);

public:
    /**
    ** get (belong to virtual function in base class) is ommited in LMCache
    **/
    virtual bool   put(const Slice & key, const Slice & value);
    virtual bool   remove(const Slice & key);
    virtual void   clear();
    /**
    ** The distinct of FIFO and LRU is the schedule policy
    ** which means who should be picked out firstly when it exceeded a limited size
    **/
    virtual Slice  getNextKey() = 0;

protected:
    const static int defaultCacheSize;
    const static int defaultCacheSizeInMB;
    const int cacheLimit;

private:
    Mutex m_mutex;

protected:
    /**
    ** Avoid useless lock(or mutex).
    **/
    Atomic cacheSize;
};

};


#endif