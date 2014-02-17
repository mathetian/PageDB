#ifndef _LM_CACHE_H
#define _LM_CACHE_H

#include <set>
#include <list>
using std::set;
using std::list;

#include "BaseCache.h"

namespace customdb{

class LimitedMemoryCache : public BaseCache
{
public:
    LimitedMemoryCache(int cacheLimitInMB = defaultCacheSizeInMB);
    
public:
    virtual bool   put(const Slice & key, const Slice & value);
    virtual bool   remove(const Slice & key);
    virtual void   clear();
    virtual Slice  removeNext() = 0;

protected:
    void           clearZero(list <Slice> &q);

protected:
    const static int defaultCacheSize;
    const static int defaultCacheSizeInMB;

private:
    const int cacheLimit;
    Atomic cacheSize;
    set<Slice> hardCache;
    Mutex m_mutex;
}; 

};


#endif