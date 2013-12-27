#ifndef _LIMIT_MEMORY_CACHE_H
#define _LIMIT_MEMORY_CACHE_H

#include <list>
using namespace std;

#include "BaseCache.h"

class LimitedMemoryCache : public BaseCache{
public:
	LimitedMemoryCache();
	LimitedMemoryCache(int cacheLimit);
	virtual ~ LimitedMemoryCache();
public:
	virtual bool   put(const string&key, const string&value);
	virtual string get(const string&key);
	virtual bool   remove(const string&key);
	virtual vector<string> keys(const string&key);
	virtual void clear();
private:
	const static int MAX_NORMAL_CACHE_SIZE;
	const static int MAX_NORMAL_CACHE_SIZE_IN_MB;
	int   cacheLimit;
	int   cacheSize;
	list<string> hardCache;
};
#endif