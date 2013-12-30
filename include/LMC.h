#ifndef _LIMIT_MEMORY_CACHE_H
#define _LIMIT_MEMORY_CACHE_H

#include <list>
#include <set>
using namespace std;

#include "BaseCache.h"
#include "Log.h"

class LimitedMemoryCache : public BaseCache{
public:
	LimitedMemoryCache();
	LimitedMemoryCache(int cacheLimitInMB);
	virtual ~ LimitedMemoryCache();
public:
	virtual bool   put(const string&key, const string&value);
	virtual string get(const string&key);
	virtual bool   remove(const string&key);
	virtual vector<string> keys(const string&key);
	virtual void   clear();
public:
	virtual string   removeNext() = 0;

private:
	const static int defaultCacheSize;
	const static int defaultCacheSizeInMB;

private:
	int              cacheLimit;
	int              cacheSize;
	set   <string>   hardCache;
	Log        *     log;
};
#endif