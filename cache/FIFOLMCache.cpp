#include "FIFOLMCache.h"

namespace customdb
{

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache(int cacheLimitInMB) :\
    LimitedMemoryCache(cacheLimitInMB) { }


bool FIFOLimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    ScopeMutex scope(&m_mutex);
    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_back(key);
        return true;
    }
    return false;
}

bool FIFOLimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    bool flag = LimitedMemoryCache::remove(key);

    if(flag == false) return false;

    {
        list <Slice>::iterator itDq = sQue.begin();

        while(itDq != sQue.end() && *itDq != key) itDq++;

        if(itDq == sQue.end())
            log -> _Warn("Not exist in FIFOLimitedMemoryCache remove");
        else
            sQue.erase(itDq);
    }

    return true;
}

Slice FIFOLimitedMemoryCache::removeNext()
{
    Slice key = sQue.front();
    sQue.pop_front();
    return key;
}

void FIFOLimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    LimitedMemoryCache::clear();
    clearZero(sQue);
}

};