#ifndef _FIFO_LIMITED_MEMORY_CACHE_H
#define _FIFO_LIMITED_MEMORY_CACHE_H

#include "LimitedMemoryCache.h"

class FIFOLimitedMemoryCache:public LimitedMemoryCache{
public:
	FIFOLimitedMemoryCache();
	FIFOLimitedMemoryCache(int sizeLimit);
	virtual ~ FIFOLimitedMemoryCache();
	virtual string removeNext();
	virtual bool put(const char&key,const char&value);
	virtual void remove(const char&key);
	virtual void clear();
private:
	queue<string> sQue;
};
#endif