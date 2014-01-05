#include "FFLMC.h"

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache() : LimitedMemoryCache()
{
}

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache(int sizeLimit) : LimitedMemoryCache(sizeLimit)
{
}

FIFOLimitedMemoryCache::~FIFOLimitedMemoryCache()
{
}

string FIFOLimitedMemoryCache::removeNext()
{
    string rs = sQue.front();
    sQue.pop_front();
    return rs;
}

bool FIFOLimitedMemoryCache::put(const string&key,const string&value)
{
    if(LimitedMemoryCache::put(key,value) == true)
    {
        sQue.push_back(value);
        return true;
    }
    return false;
}

string FIFOLimitedMemoryCache::get(const string&key)
{
    string value = LimitedMemoryCache::get(key);

    deque <string>::iterator itDq = sQue.begin();

    for(; itDq < sQue.end(); itDq++)
    {
        if(*itDq == key) break;
    }

    sQue.erase(itDq);
    sQue.push_back(value);

    return value;
}

bool FIFOLimitedMemoryCache::remove(const string&key)
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

void FIFOLimitedMemoryCache::clear()
{
    LimitedMemoryCache::clear();
    clearQ(sQue);
}