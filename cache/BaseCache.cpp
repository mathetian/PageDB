#include "BaseCache.h"

BaseCache::BaseCache()
{
}

BaseCache::~BaseCache()
{
}

bool BaseCache::put(const string&key,const string&value)
{
    if(softMap.find(key) == softMap.end())
    {
        softMap[key] = value;
        return true;
    }
    return false;
}

string BaseCache::get(const string&key)
{
    if(softMap.find(key) == softMap.end())
        return "";
    return softMap[key];
}

bool BaseCache::remove(const string&key)
{
    return softMap.erase(key);
}

vector<string> BaseCache::keys()
{
    map<string, string>::iterator itMap = softMap.begin();
    vector<string> rsv;
    for(; itMap != softMap.end(); itMap++)
        rsv.push_back(itMap -> first);
    return rsv;
}

void BaseCache::clear()
{
    softMap.erase(softMap.begin(),softMap.end());
}