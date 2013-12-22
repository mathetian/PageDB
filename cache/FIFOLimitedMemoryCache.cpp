#include "FIFOLimitedMemoryCache.h"

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache()
{
	LimitedMemoryCache::LimitedMemoryCache();
}

FIFOLimitedMemoryCache::FIFOLimitedMemoryCache(int sizeLimit)
{
	LimitedMemoryCache::LimitedMemoryCache(sizeLimit);
}

FIFOLimitedMemoryCache::~FIFOLimitedMemoryCache()
{
}

string FIFOLimitedMemoryCache::removeNext()
{
	return sQue.front();
}

bool FIFOLimitedMemoryCache::put(const string&key,const string&value)
{
	if(LimitedMemoryCache::put(key,value))
	{
		sQue.push(value);
		return true;
	}
	return false;
}

void FIFOLimitedMemoryCache::remove(const string&key)
{
	const string&value=LimitedMemoryCache::get(key);
	LimitedMemoryCache::remove(key);
	sQue.remove(value);
}

void FIFOLimitedMemoryCache::clear()
{
	LimitedMemoryCache::clear();
	sQue.clear();
}