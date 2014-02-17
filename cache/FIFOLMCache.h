#ifndef _FIFO_LMC_H
#define _FIFO_LMC_H

#include "LMCache.h"

namespace customdb
{

class FIFOLimitedMemoryCache : public LimitedMemoryCache
{
public:
    FIFOLimitedMemoryCache(int cacheLimitInMB = LimitedMemoryCache::defaultCacheSizeInMB);

public:
    bool    put(const Slice & key, const Slice & value);
    bool    remove(const Slice & key);
    void    clear();
    Slice   removeNext();

private:
    list <Slice> sQue;
    Mutex        m_mutex;
};

};

#endif