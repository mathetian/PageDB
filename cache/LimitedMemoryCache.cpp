#include "LimitedMemoryCache.h"

const int LimitedMemoryCache::MAX_NORMAL_CACHE_SIZE=16;
const int LimitedMemoryCache::MAX_NORMAL_CACHE_SIZE_IN_MB=MAX_NORMAL_CACHE_SIZE*1024*1024;

LimitedMemoryCache::LimitedMemoryCache()
{
	this->cacheLimit=MAX_NORMAL_CACHE_SIZE;
	this->cacheSize=0;
}

LimitedMemoryCache::LimitedMemoryCache(int cacheLimit)
{
	this->cacheLimit=cacheLimit;
	this->cacheSize=0;
	if(cacheSize>MAX_NORMAL_CACHE_SIZE)
		Log::w("LimitedMemoryCache: MAX_NORMAL_CACHE_SIZE\n");
}

bool LimitedMemoryCache::put(const string&key,const string&value)
{
	int valueSize = value.size();
	int sizeLimit = sizeLimit;
	int curCacheSize = cacheSize;
	bool putSuccessfully=false;
	if(valueSize<sizeLimit) 
	{
		while (curCacheSize + valueSize > sizeLimit) 
		{
				string removedValue = removeNext();
				if (hardCache.remove(removedValue)) 
				{
					curCacheSize = cacheSize-removedValue.size();
				}
		}
		hardCache.push_back(value);
		cacheSize+=curCacheSize;
		putSuccessfully = true;
		BaseCache::put(key,hardCache[hardCache.size()-1]);
	}
	else Log::w("Large size\n");
	return putSuccessfully;
}

void LimitedMemoryCache::remove(const string&key)
{
	const string&removedValue=BaseCache::get(key);
	cacheSize-=value.size();
	hardCache.remove(removedValue);
	BaseCache::remove(key);
}

void LimitedMemoryCache::clear()
{
	cacheSize=0;hardCache.clear();
	BaseCache::clear();
}

