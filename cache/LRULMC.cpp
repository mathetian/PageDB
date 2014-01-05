#include "LRULMC.h"

LRULimitedMemoryCache::LRULimitedMemoryCache() : LimitedMemoryCache()
{
}

LRULimitedMemoryCache::LRULimitedMemoryCache(int sizeLimit) : LimitedMemoryCache(sizeLimit)
{
}

LRULimitedMemoryCache::~LRULimitedMemoryCache()
{
}

string LRULimitedMemoryCache::removeNext()
{
    string rs = sQue.front();
    sQue.pop_front();
    return rs;
}

bool  LRULimitedMemoryCache::put(const string&key, const string&value)
{
    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_back(value);
        return true;
    }
    return false;
}

bool   LRULimitedMemoryCache::remove(const string&key)
{
    LimitedMemoryCache::remove(key);
    deque <string>::iterator itDq = sQue.begin();
    for(; itDq < sQue.end(); itDq++)
    {
        if(*itDq == key) break;
    }
    sQue.erase(itDq);
    return true;
}

string LRULimitedMemoryCache::get(const string&key)
{

}

void   LRULimitedMemoryCache::clear()
{
    LimitedMemoryCache::clear();
    clearQ(sQue);
}