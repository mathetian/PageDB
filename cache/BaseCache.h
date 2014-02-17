#ifndef _BASECACHE_H
#define _BASECACHE_H

#include <map>
#include <vector>
using std::map;
using std::vector;

#include "../include/Log.h"
#include "../include/Slice.h"

#include "../utils/Thread.h"
using utils::Mutex;
using utils::Atomic;
using utils::ScopeMutex;

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