#include "baseCache.h"

bool BaseCache::put(const string&key,const string&value)
{
	if(softMap.find(key)==softMap::end())
	{
		softMap.put(key,value);
		return true;
	}	
	return false;
}

string BaseCache::get(const string&key)
{
	if(softMap.find(key)==softMap.end())
		return "";
	return softMap.get(key);
}

void BaseCache::remove(const string&key)
{
	softMap.remove(key);
}

vector<string> BaseCache::keys()
{
	return vector<string>(softMap.keySet());
}

void clear()
{
	softMap.clear();
}