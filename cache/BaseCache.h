#ifndef _BASECACHE_H
#define _BASECACHE_H

#include <map>
#include <vector>
using std::map;
using std::vector;

#include "Log.h"
#include "Slice.h"

#include "Thread.h"
using utils::Mutex;
using utils::ScopeMutex;
#include "Atomic.h"
using utils::Atomic;

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