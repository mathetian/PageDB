#ifndef _BASE_CACHE_H
#define _BASE_CACHE_H

#include <map>
#include <set>
#include <list>
#include <queue>
#include <vector>
#include <algorithm>
using namespace std;

#include "Log.h"
#include "Slice.h"
#include "Thread.h"

class BaseCache
{
public:
    BaseCache() {  log = Log::GetInstance();  }
    virtual ~ BaseCache() { }
    
public:
    virtual bool    put(const Slice & key, const Slice & value)
    {
        ScopeMutex scope(&m_mutex);

        if(softMap.find(key) == softMap.end())
        {
            softMap[key] = value;
            return true;
        }
        return false;
    }
    
    virtual Slice  	get(const Slice & key)
    {
        ScopeMutex scope(&m_mutex);
        
        if(softMap.find(key) == softMap.end())
            return "";
        return softMap[key];
    }

    virtual bool    remove(const Slice & key) { return softMap.erase(key); }
    
    vector<Slice>   keys()
    {
        ScopeMutex scope(&m_mutex);
        
        map<Slice, Slice>::iterator itMap = softMap.begin();
        vector<Slice> rsv;
        for(; itMap != softMap.end(); itMap++)
            rsv.push_back(itMap -> first);
        return rsv;
    }

    virtual void    clear()
    {
        ScopeMutex scope(&m_mutex);

        softMap.erase(softMap.begin(),softMap.end());
    }

protected:
	Log   * log;
	
private:
    map<Slice, Slice> softMap;
    Mutex m_mutex;
};

class LimitedMemoryCache : public BaseCache
{
public:
    LimitedMemoryCache(int cacheLimitInMB = defaultCacheSizeInMB):\
        cacheLimit(cacheLimitInMB * 1024 * 1024), cacheSize(0), BaseCache()
    {
        if(cacheLimit > defaultCacheSize)
            log -> _Warn("LimitedMemoryCache: defaultCacheSize\n");
    }
    
    virtual ~ LimitedMemoryCache() { }

public:
    virtual bool   put(const Slice & key, const Slice & value);
    virtual bool   remove(const Slice & key);
    virtual void   clear();

public:
    virtual Slice  removeNext() = 0;

protected:
    void           clearZero(list <Slice> &q);

protected:
    const static int defaultCacheSize;
    const static int defaultCacheSizeInMB;

private:
    const int cacheLimit;
    Atomic cacheSize;
    set <Slice> hardCache;
    Mutex m_mutex;
};

class FIFOLimitedMemoryCache : public LimitedMemoryCache
{
public:
    FIFOLimitedMemoryCache(int cacheLimitInMB = LimitedMemoryCache::defaultCacheSizeInMB) :\
        LimitedMemoryCache(cacheLimitInMB)
         { }
    
    ~FIFOLimitedMemoryCache() { }

public:
    bool    put(const Slice & key, const Slice & value);
    bool    remove(const Slice & key);
    void    clear();

public:
    Slice   removeNext();

private:
    list <Slice> sQue;
    Mutex        m_mutex;
};

class LRULimitedMemoryCache : public LimitedMemoryCache
{
public:
    LRULimitedMemoryCache(int cacheLimitInMB = LimitedMemoryCache::defaultCacheSizeInMB) :\
        LimitedMemoryCache(cacheLimitInMB) { }
   
   ~LRULimitedMemoryCache() { }

public:
    bool    put(const Slice & key, const Slice & value);
    Slice   get(const Slice & key);
    bool    remove(const Slice & key);
    void    clear();

public:
    Slice   removeNext();

private:
    list <Slice> sQue;
    Mutex       m_mutex;
};

#endif