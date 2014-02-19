#include "LMCache.h"

namespace customdb
{

const int LimitedMemoryCache::defaultCacheSizeInMB=16;
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
        while (curCacheSize + valueSize > cacheLimit)
        {
            Slice removedKey   = removeNext();
            Slice removedValue = BaseCache::get(removedKey);
           
            if (hardCache.erase(removedKey))
            {
                curCacheSize = cacheSize.addAndGet(-removedValue.size());
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

};