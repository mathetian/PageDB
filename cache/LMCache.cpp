// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "LMCache.h"

namespace customdb
{

/**
** Cache upper bound: 16MB, which means that we can't see the value bigger than that
**/
const int LimitedMemoryCache::defaultCacheSizeInMB = 16;
const int LimitedMemoryCache::defaultCacheSize = defaultCacheSizeInMB * 1024 * 1024;

LimitedMemoryCache::LimitedMemoryCache(int cacheLimitInMB) :\
    cacheLimit(cacheLimitInMB * 1024 * 1024), cacheSize(0), BaseCache()
{
    if(cacheLimit > defaultCacheSize)
        log -> _Warn("LimitedMemoryCache: defaultCacheSize\n");
}

bool LimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    int  valueSize       = value.size();
    bool putSuccessfully = false;

    int  curCacheSize    = cacheSize;

    if(valueSize < cacheLimit)
    {
        bool flag = BaseCache::put(key, value);
        /**
        ** true:  new key
        ** false: duplicate key, overwrite
        **/
        if(flag == true)
        {
            while (curCacheSize + valueSize > cacheLimit)
            {
                Slice removedKey   = getNextKey();
                Slice removedValue = BaseCache::get(removedKey);

                curCacheSize = cacheSize.addAndGet(-removedValue.size());
            }

            cacheSize.addAndGet(valueSize);

            putSuccessfully = true;
        }
        else
        {
        }
    }
    else log -> _Warn("Too large size, can't be put into cache\n");

    return putSuccessfully;
}

bool LimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    const Slice & removedValue = BaseCache::get(key);

    if(removedValue.size() == 0) return false;

    cacheSize -= removedValue.size();

    BaseCache::remove(key);

    return true;
}

void LimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    BaseCache::clear();
    cacheSize = 0;
}

};