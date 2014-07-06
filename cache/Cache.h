// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Log.h"
#include "Slice.h"
#include "Multithreading.h"
using namespace utils;

namespace cache
{

class Cache
{
public:
    Cache(int slotnum) : slotnum_(slotnum), \
        header_(NULL), ender_(NULL)
    {
        header_ = new Entry;
        ender_  = new Entry;

        header_ -> next_ = ender_;
        ender_  -> prev_ = header_;

        assert(slotnum_ > 0);
    }

    virtual ~Cache()
    {
        clear();

        delete header_;
        delete ender_;

        header_ = ender_ = NULL;
    }

public:
    /// Base cache support put/get/remove
    /// All operation are operated on softMap

    virtual bool    put(const Slice &key, const Slice &value) = 0;
    virtual Slice  	get(const Slice &key) = 0;
    virtual bool    remove(const Slice &key) = 0;

public:
    /// Clear Operation
    void    clear()
    {
        Entry *entry = header_ -> next_;

        while(entry != ender_)
        {
            Entry *cur   = entry;
            entry = cur -> next_;
            delete cur;
            cur = NULL;
        }

        header_ -> next_ = ender_;
        ender_  -> prev_ = header_;

        dict_.clear();
    }

private:
    void insert(Entry *entry)
    {
        Slice key = entry -> key_;

        entry_ -> next_ = header_ -> next_;
        entry_ -> prev_ = header_;

        header_ -> next_ -> prev_ = entry_;
        header_ -> next_ = entry_;

        dict_[key]   = entry;
        slotnum_++;
    }

    void remove(Entry *entry)
    {
        Slice key = entry -> key_;

        entry -> prev_ -> next_ = entry -> next_;
        entry -> next_ -> prev  = entry -> prev_;

        delete entry;
        entry_ = NULL;

        dict_.erase(key);
        slotnum_--;
    }

protected:
    struct Entry_t
    {
        Slice key_, value_;
        struct Entry_t *next_;
        struct Entry_t *prev_;

        Entry_t(const Slice &key, const Slice &value)
        {
            key_ = key_;
            value_ = value;
            next_ = prev_ = NULL;
        }
    };
    typedef struct Entry_t Entry;

protected:
    const int          slotnum_;
    map<Slice, Entry*> dict_;
    Entry             *header_;
    Entry_t           *ender_;
};

};