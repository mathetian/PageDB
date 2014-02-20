#ifndef _EMPTY_CACHE_H
#define _EMPTY_CACHE_H

#include "BaseCache.h"

namespace customdb
{

class EmptyCache : public BaseCache
{
public:
    EmptyCache() { }

public:
    bool    put(const Slice & key, const Slice & value) { return true; }
    Slice  	get(const Slice & key) { return BaseCache::get(key); }
    bool    remove(const Slice & key) { return true; }
    void    clear() { }
};

};

#endif