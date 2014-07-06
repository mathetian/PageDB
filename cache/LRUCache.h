// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Cache.h"

namespace cache
{

class LRUCache : public Cache
{
public:
    LRUCache(int slotnum) : Cache(slotnum)
    {
    }

    virtual ~LRUCache()
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
                remove(ender_ -> prev);
            }

            insert(new Entry(key, value));
        }
        else
        {
            /// Replace
            remove(dict_[key]);
            insert(new Entry(key, value));
        }
    }

    virtual Slice  	get(const Slice &key)
    {
        if(dict_.find(key) == dict_.end())
            return "";

        Slice value = dict_[key].value_;

        remove(dict_[key]);
        insert(new Entry(key, value));
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
