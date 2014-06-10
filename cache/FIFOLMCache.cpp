// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "FIFOLMCache.h"

namespace customdb
{

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache(int cacheLimitInMB) :\
    LimitedMemoryCache(cacheLimitInMB) { }

bool FIFOLimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    if(LimitedMemoryCache::put(key, value) == true)
    {
        m_list.push_back(key);
        return true;
    }
    else
    {
        if(value.size() < cacheLimit)
        {
            /**
            ** Reorder the key
            **/
            reorder(key);
            return true;
        }
    }

    return false;
}

bool FIFOLimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    bool flag = LimitedMemoryCache::remove(key);

    if(flag == false) return false;

    {
        list <Slice>::iterator itDq = m_list.begin();

        while(itDq != m_list.end() && *itDq != key) itDq++;

        if(itDq == m_list.end())
            log -> _Warn("Not exist in FIFOLimitedMemoryCache remove");
        else
            m_list.erase(itDq);
    }

    return true;
}

Slice FIFOLimitedMemoryCache::getNextKey()
{
    Slice key = m_list.front();
    m_list.pop_front();

    return key;
}

void FIFOLimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    LimitedMemoryCache::clear();

    list<Slice> tmp;
    swap(tmp, m_list);
}

void FIFOLimitedMemoryCache::reorder(const Slice & key)
{
    list<Slice>::iterator iter = m_list.begin();

    while(iter != m_list.end() && *iter != key) iter++;

    assert(iter != m_list.end());
    m_list.erase(iter);
    m_list.push_back(key);
}

};