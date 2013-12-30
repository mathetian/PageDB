#include "../include/FFLMC.h"

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
	return sQue.front();
}

bool FIFOLimitedMemoryCache::put(const string&key,const string&value)
{
	if(LimitedMemoryCache::put(key,value) == true)
	{
		sQue.push_back(key);
		return true;
	}
	return false;
}

bool FIFOLimitedMemoryCache::remove(const string&key)
{
	LimitedMemoryCache::remove(key);
	deque <string>::iterator itDq = sQue.begin();
	for(;itDq < sQue.end();itDq++)
	{ if(*itDq == key) break; }
	sQue.erase(itDq);
	return true;
}

void clearQ(deque <string> &q )
{
   deque<string> empty;
   swap(q, empty);
}

void FIFOLimitedMemoryCache::clear()
{
	LimitedMemoryCache::clear();
	clearQ(sQue);
}