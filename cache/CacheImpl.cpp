#include "CacheImpl.h"

const int LimitedMemoryCache::defaultCacheSizeInMB=16;
const int LimitedMemoryCache::defaultCacheSize = defaultCacheSizeInMB * 1024 * 1024;

bool LimitedMemoryCache::put(const Slice & key,const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    int  valueSize       = value.size();
    bool putSuccessfully = false;

    int  curCacheSize    = cacheSize;

    if(valueSize < cacheLimit)
    {
        while (curCacheSize + valueSize > cacheLimit)
        {
            Slice removedKey   = removeNext();
            Slice removedValue = BaseCache::get(removedKey);

            if (hardCache.erase(removedKey))
            {
                curCacheSize = cacheSize.addAndGet(removedValue.size());
            }
        }

        hardCache.insert(key);

        cacheSize.addAndGet(valueSize);

        putSuccessfully = true;
        BaseCache::put(key, value);
    }
    else log -> _Warn("Large size, can't be put into cache\n");
    
    return putSuccessfully;
}

bool LimitedMemoryCache::remove(const Slice & key)
{
    ScopeMutex scope(&m_mutex);
        
    const Slice & removedValue = BaseCache::get(key);
    
    if(removedValue.size() == 0) return false;
    
    BaseCache::remove(key);

    cacheSize -= removedValue.size();
    
    return hardCache.erase(key);
}

void LimitedMemoryCache::clear()
{
    ScopeMutex scope(&m_mutex);

    cacheSize = 0;
    hardCache.clear();
    BaseCache::clear();
}

void LimitedMemoryCache::clearZero(list <Slice> &q )
{
    ScopeMutex scope(&m_mutex);
        
    list <Slice> empty;
    swap(q, empty);
}

/***End LMCache**/

/**Begin FIFOLMCache**/

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
    ScopeMutex scope(&m_mutex);

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

/**End FIFOLMCache**/

/**Begin LRULMCache**/

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
