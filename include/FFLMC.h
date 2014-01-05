#ifndef _FIFO_LIMITED_MEMORY_CACHE_H
#define _FIFO_LIMITED_MEMORY_CACHE_H

#include <queue>
using namespace std;

#include "LMC.h"

class FIFOLimitedMemoryCache : public LimitedMemoryCache
{
public:
    FIFOLimitedMemoryCache();
    FIFOLimitedMemoryCache(int sizeLimit);
    ~ FIFOLimitedMemoryCache();
public:
    string  removeNext();
    bool    put(const string & key, const string & value);
    bool    remove(const string & key);
    string	get(const string & key);
    void    clear();

private:
    deque <string> sQue;
    typedef LimitedMemoryCache Base;
};

#endif