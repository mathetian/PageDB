#include "../include/BaseCache.h"

const int LimitedMemoryCache::defaultCacheSizeInMB=16;
const int LimitedMemoryCache::defaultCacheSize = defaultCacheSizeInMB * 1024 * 1024;

bool LimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    int  valueSize       = value.size();
    bool putSuccessfully = false;
    
    if(valueSize < cacheLimit)
    {
        while (cacheSize + valueSize > cacheLimit)
        {
            Slice removedKey = removeNext();
            Slice removedValue = BaseCache::get(removedKey);

            if (hardCache.erase(removedKey))
            {
                cacheSize -= removedValue.size();
            }
        }

        hardCache.insert(key);
        cacheSize += valueSize;
        putSuccessfully = true;
        BaseCache::put(key, value);
    }
    else log -> _Warn("Large size, can't be put into cache\n");
    
    return putSuccessfully;
}

Slice LimitedMemoryCache::get(const Slice & key)
{
    return BaseCache::get(key);
}

bool LimitedMemoryCache::remove(const Slice & key)
{
    const Slice & removedValue = BaseCache::get(key);
    if(removedValue.size() == 0) return false;
    
    cacheSize -= removedValue.size();
    
    return hardCache.erase(key);
}

void LimitedMemoryCache::clear()
{
    cacheSize = 0;
    hardCache.clear();
    BaseCache::clear();
}

void LimitedMemoryCache::clearZero(deque <Slice> &q )
{
    deque <Slice> empty;
    swap(q, empty);
}

/***End LMCache**/

/**Begin FIFOLMCache**/

bool FIFOLimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_back(value);
        return true;
    }
    return false;
}

Slice FIFOLimitedMemoryCache::get(const Slice & key)
{
    return LimitedMemoryCache::get(key);
}

bool FIFOLimitedMemoryCache::remove(const Slice & key)
{
    bool flag = LimitedMemoryCache::remove(key);

    if(flag == false) return false;

    deque <Slice>::iterator itDq = sQue.begin();
    
    while(itDq != sQue.end() && *itDq == key) itDq++;
   
    if(itDq == sQue.end())
        log -> _Warn("Not exist in FIFOLimitedMemoryCache remove\n");
    else   
        sQue.erase(itDq);
   
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
    LimitedMemoryCache::clear();
    clearZero(sQue);
}

/**End FIFOLMCache**/

/**Begin LRULMCache**/

bool  LRULimitedMemoryCache::put(const Slice & key, const Slice & value)
{
    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_front(value);
        return true;
    }
    return false;
}

Slice LRULimitedMemoryCache::get(const Slice & key)
{
    Slice value = LimitedMemoryCache::get(key);
    
    deque <Slice>::iterator itDq = sQue.begin();

    while(itDq != sQue.end() && *itDq == key) itDq++;
    
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
    bool flag = LimitedMemoryCache::remove(key);
    if(flag == false) return false;

    deque <Slice>::iterator itDq = sQue.begin();
    while(itDq != sQue.end() && *itDq == key) itDq++;

    if(itDq == sQue.end())
        log -> _Warn("Not exist in LRULimitedMemoryCache remove?\n");
    else
        sQue.erase(itDq);
    
    return true;
}

void   LRULimitedMemoryCache::clear()
{
    LimitedMemoryCache::clear();
    clearZero(sQue);
}

Slice LRULimitedMemoryCache::removeNext()
{
    Slice rs = sQue.front();
    sQue.pop_front();
    return rs;
}
