#include "../include/LMC.h"

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

bool LimitedMemoryCache::put(const string&key,const string&value)
{
	int  keySize = key.size();
	bool putSuccessfully = false;
	if(valueSize < cacheLimit) 
	{
		while (cacheSize + keySize > cacheLimit) 
		{
			string removedValue = removeNext();
			if (hardCache.erase(removedValue)) 
			{
				cacheSize -= removedValue.size();
			}
		}

		hardCache.insert(key);
		cacheSize += keySize;
		putSuccessfully = true;
		BaseCache::put(key, value);
	}
	else log -> _Warn("Large size\n");
	return putSuccessfully;
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

