#ifndef _BASECACHE_H
#define _BASECACHE_H

#include <map>
#include <vector>
using namespace std;

#include "Log.h"
using namespace customdb;

#include "Slice.h"
#include "Multithreading.h"
#include "Atomic.h"
using namespace utils;

namespace customdb
{

class BaseCache
{
public:
    BaseCache();

public:
    virtual bool    put(const Slice & key, const Slice & value);
    virtual Slice  	get(const Slice & key);
    virtual bool    remove(const Slice & key);
    vector<Slice>   keys();
    virtual void    clear();

protected:
    Log   * log;

private:
    map<Slice, Slice> softMap;
    Mutex m_mutex;
};

};

#endif