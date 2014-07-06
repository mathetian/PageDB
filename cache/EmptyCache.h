// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _EMP_CACHE_H
#define _EMP_CACHE_H

#include "Cache.h"

namespace cache
{

class EmptyCache : public Cache
{
public:
    EmptyCache() : Cache(1)
    {
    }

    virtual ~EmptyCache()
    {
    }

public:
    virtual bool    put(const Slice & key, const Slice & value)
    {
        return true;
    }

    virtual Slice  	get(const Slice & key)
    {
        return "";
    }

    virtual bool    remove(const Slice & key)
    {
        return true;
    }
};

};

#endif