#include "LRULMCache.h"

namespace customdb{

LRULimitedMemoryCache::LRULimitedMemoryCache(int cacheLimitInMB) :\
    LimitedMemoryCache(cacheLimitInMB) { }

bool  LRULimitedMemoryCache::put(const Slice & key, const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_front(key);
        return true;
    }
    return false;
}

Slice LRULimitedMemoryCache::get(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    Slice value = LimitedMemoryCache::get(key);
    
    list <Slice>::iterator itDq = sQue.begin();

    while(itDq != sQue.end() && *itDq != key) itDq++;
    
    if(itDq != sQue.end())
    {
        sQue.erase(itDq);
        sQue.push_front(key);
    }
    else log -> _Warn("LRULimitedMemoryCache get not exist?\n");

    return value;
}

bool   LRULimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    bool flag = LimitedMemoryCache::remove(key);
    if(flag == false) return false;

    list <Slice>::iterator itDq = sQue.begin();
    while(itDq != sQue.end() && *itDq != key) itDq++;

    if(itDq == sQue.end())
        log -> _Warn("Not exist in LRULimitedMemoryCache remove?\n");
    else
        sQue.erase(itDq);
    
    return true;
}

void   LRULimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    LimitedMemoryCache::clear();
    clearZero(sQue);
}

Slice LRULimitedMemoryCache::removeNext()
{
    ScopeMutex scope(&m_mutex);

    Slice rs = sQue.front();
    sQue.pop_front();
    return rs;
}


};