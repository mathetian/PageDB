#include "LMC.h"

const int LimitedMemoryCache::defaultCacheSizeInMB=16;
const int LimitedMemoryCache::defaultCacheSize = defaultCacheSizeInMB * 1024 * 1024;

LimitedMemoryCache::LimitedMemoryCache()
{
    this->cacheLimit = defaultCacheSize;
    this->cacheSize = 0;
    log = Log::GetInstance();
}

LimitedMemoryCache::LimitedMemoryCache(int cacheLimitInMB)
{
    this -> cacheLimit = cacheLimit * 1024 * 1024;
    this -> cacheSize  = 0;
    log = Log::GetInstance();
    if(cacheSize > defaultCacheSize)
        log -> _Warn("LimitedMemoryCache: defaultCacheSize\n");
}

LimitedMemoryCache::~LimitedMemoryCache()
{
}

bool LimitedMemoryCache::put(const string&key,const string&value)
{
    int  valueSize = value.size();
    bool putSuccessfully = false;
    if(valueSize < cacheLimit)
    {
        while (cacheSize + valueSize > cacheLimit)
        {
            string removedValue = removeNext();
            if (hardCache.erase(removedValue))
            {
                cacheSize -= removedValue.size();
            }
        }

        hardCache.insert(value);
        cacheSize += valueSize;
        putSuccessfully = true;
        BaseCache::put(key, value);
    }
    else log -> _Warn("Large size\n");
    return putSuccessfully;
}

string LimitedMemoryCache::get(const string&key)
{
    return BaseCache::get(key);
}

bool LimitedMemoryCache::remove(const string&key)
{
    const string&removedValue = BaseCache::get(key);
    cacheSize -= key.size();
    return hardCache.erase(key);
}

void LimitedMemoryCache::clear()
{
    cacheSize=0;
    hardCache.clear();
    BaseCache::clear();
}

void LimitedMemoryCache::clearQ(deque <string> &q )
{
    deque<string> empty;
    swap(q, empty);
}


