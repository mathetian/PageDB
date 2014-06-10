// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _FIFO_LMC_H
#define _FIFO_LMC_H

#include "LMCache.h"

namespace customdb
{

class FIFOLimitedMemoryCache : public LimitedMemoryCache
{
public:
    FIFOLimitedMemoryCache(int cacheLimitInMB = LimitedMemoryCache::defaultCacheSizeInMB);

public:
    bool    put(const Slice & key, const Slice & value);
    bool    remove(const Slice & key);
    void    clear();
    Slice   getNextKey();

private:
    void    reorder(const Slice & key);

private:
    list <Slice> m_list;
    Mutex        m_mutex;
};

};

#endif