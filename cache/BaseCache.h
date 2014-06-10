// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _BASECACHE_H
#define _BASECACHE_H

#include "Log.h"
#include "Slice.h"
#include "Atomic.h"
#include "Multithreading.h"
using namespace utils;

/**
** BaseCache is the base class of Cache module
** It is thread-safe
**/
namespace customdb
{

class BaseCache
{
public:
    BaseCache();

public:
    /**
    ** Base cache support put/get/remove/clear
    ** All operation are operated on softMap
    **/
    virtual bool    put(const Slice & key, const Slice & value);
    virtual Slice  	get(const Slice & key);
    virtual bool    remove(const Slice & key);
    virtual void    clear();

protected:
    Log   * log;

private:
    map<Slice, Slice> softMap;
    Mutex m_mutex;
};

};

#endif