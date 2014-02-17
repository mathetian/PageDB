#ifndef _LRU_LMC_H
#define _LRU_LMC_H

#include "LMCache.h"

namespace customdb{

class LRULimitedMemoryCache : public LimitedMemoryCache
{
public:
    LRULimitedMemoryCache(int cacheLimitInMB = LimitedMemoryCache::defaultCacheSizeInMB);

public:
    bool    put(const Slice & key, const Slice & value);
    Slice   get(const Slice & key);
    bool    remove(const Slice & key);
    void    clear();
    Slice   removeNext();

private:
    list <Slice> sQue;
    Mutex       m_mutex;
};

};
#endif