// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "BaseCache.h"

namespace customdb
{

BaseCache::BaseCache()
{
    log = Log::GetInstance();
}

bool   BaseCache::put(const Slice & key, const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    if(softMap.find(key) == softMap.end())
    {
        softMap[key] = Slice(value.c_str(), value.size());
        return true;
    }

    softMap[key] = Slice(value.c_str(), value.size());

    return false;
}

Slice  BaseCache::get(const Slice& key)
{
    ScopeMutex scope(&m_mutex);

    if(softMap.find(key) == softMap.end())
        return "";

    return softMap[key];
}

bool BaseCache::remove(const Slice & key)
{
    return softMap.erase(key);
}

void   BaseCache::clear()
{
    ScopeMutex scope(&m_mutex);
    map<Slice, Slice> tmp;
    swap(softMap, tmp);
}

};


