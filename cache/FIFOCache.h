// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _FIFO_CACHE_H
#define _FIFO_CACHE_H

namespace cache
{

class FIFOCache
{
public:
    FIFOCache(int slotnum) : Cache(slotnum)
    {
    }

    virtual ~FIFOCache()
    {
    }

public:
    virtual bool    put(const Slice &key, const Slice &value)
    {
        if(dict_.find(key) == dict_.end())
        {
            /// Insert
            if(dict_.size() == slotnum_)
            {
                /// Remove the oldest one
                remove(ender_ -> prev_);
            }

            insert(new Entry(key, value));
        }
        else
        {
            /// Replace
            Entry *entry    = dict_[key];
            entry -> value_ = value;
        }
    }

    virtual Slice  	get(const Slice &key)
    {
        if(dict_.find(key) == dict_.end())
            return "";

        return dict_[key] -> value_;
    }

    virtual bool    remove(const Slice &key)
    {
        if(dict_.find(key) == dict_.end())
            return false;

        remove(dict_[key]);
        return true;
    }
};

};

#endif