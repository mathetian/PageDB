#include "BaseCache.h"

namespace customdb
{

BaseCache::BaseCache()
{
    log = Log::GetInstance();
}

bool   BaseCache::put(const Slice & key, const Slice & value)
{
    ScopeMutex scope(&m_mutex);

    if(softMap.find(key) == softMap.end())
    {
        softMap[key] = value;
        return true;
    }

    return false;
}

Slice  BaseCache::get(const Slice & key)
{
    ScopeMutex scope(&m_mutex);

    if(softMap.find(key) == softMap.end())
        return "";

    return softMap[key];
}

bool BaseCache::remove(const Slice & key)
{
    return softMap.erase(key);
}

vector<Slice>  BaseCache::keys()
{
    ScopeMutex scope(&m_mutex);

    map<Slice, Slice>::iterator itMap = softMap.begin();
    vector<Slice> rsv;
    for(; itMap != softMap.end(); itMap++)
        rsv.push_back(itMap -> first);
    return rsv;
}

void   BaseCache::clear()
{
    ScopeMutex scope(&m_mutex);
    softMap.erase(softMap.begin(),softMap.end());
}

};


