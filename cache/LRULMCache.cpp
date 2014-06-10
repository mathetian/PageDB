// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "LRULMCache.h"

namespace customdb
{

LRULimitedMemoryCache::LRULimitedMemoryCache(int cacheLimitInMB) :\
    LimitedMemoryCache(cacheLimitInMB) { }

bool  LRULimitedMemoryCache::put(const Slice & key, const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    if(LimitedMemoryCache::put(key,value) == true)
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

Slice LRULimitedMemoryCache::get(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    Slice value = LimitedMemoryCache::get(key);

    list <Slice>::iterator itDq = m_list.begin();

    while(itDq != m_list.end() && *itDq != key) itDq++;

    if(itDq != m_list.end())
    {
        m_list.erase(itDq);
        m_list.push_back(key);
    }
    else log -> _Warn("LRULimitedMemoryCache get not exist?\n");

    return value;
}

bool   LRULimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    bool flag = LimitedMemoryCache::remove(key);
    if(flag == false) return false;

    list <Slice>::iterator iter = m_list.begin();
    while(iter != m_list.end() && *iter != key) iter++;

    if(iter == m_list.end())
        log -> _Warn("Not exist in LRULimitedMemoryCache remove?\n");
    else
        m_list.erase(iter);

    return true;
}

void   LRULimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    LimitedMemoryCache::clear();

    list<Slice> tmp;
    swap(tmp, m_list);
}

Slice LRULimitedMemoryCache::getNextKey()
{
    ScopeMutex scope(&m_mutex);

    Slice rs = m_list.front();
    m_list.pop_front();

    return rs;
}

void LRULimitedMemoryCache::reorder(const Slice & key)
{
    list<Slice>::iterator iter = m_list.begin();

    while(iter != m_list.end() && *iter != key) iter++;

    assert(iter != m_list.end());
    m_list.erase(iter);
    m_list.push_back(key);
}

};