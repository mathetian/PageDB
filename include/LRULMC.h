#ifndef _LRM_LMC_H
#define _LRM_LMC_H

#include <queue>
using namespace std;

#include "LMC.h"

class LRULimitedMemoryCache : public LimitedMemoryCache{
public:
	LRULimitedMemoryCache();
	LRULimitedMemoryCache(int sizeLimit);
	~ LRULimitedMemoryCache();

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