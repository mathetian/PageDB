// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CACHE_H
#define _CACHE_H

#include "Slice.h"
using namespace utils;

namespace cache
{

class Cache
{
protected:
    struct Entry_t {
        Slice key_, value_;
        struct Entry_t *next_;
        struct Entry_t *prev_;

        Entry_t(const Slice &key, const Slice &value) {
            key_ = key_;
            value_ = value;
            next_ = prev_ = NULL;
        }
    };

    typedef struct Entry_t Entry;

public:
    Cache(int slotnum) : slotnum_(slotnum), \
        header_(NULL), ender_(NULL) {
        header_ = new Entry("", "");
        ender_  = new Entry("", "");

        header_ -> next_ = ender_;
        ender_  -> prev_ = header_;

        assert(slotnum_ > 0);
    }

    virtual ~Cache() {
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
    void    clear() {
        Entry *entry = header_ -> next_;

        while(entry != ender_) {
            Entry *cur   = entry;
            entry = cur -> next_;
            delete cur;
            cur = NULL;
        }

        header_ -> next_ = ender_;
        ender_  -> prev_ = header_;

        dict_.clear();
    }

protected:
    void insert(Entry *entry) {
        Slice key = entry -> key_;

        entry -> next_ = header_ -> next_;
        entry -> prev_ = header_;

        header_ -> next_ -> prev_ = entry;
        header_ -> next_ = entry;

        dict_[key]   = entry;
    }

    void del(Entry *entry) {
        Slice key = entry -> key_;

        entry -> prev_ -> next_ = entry -> next_;
        entry -> next_ -> prev_ = entry -> prev_;

        delete entry;
        entry = NULL;

        dict_.erase(key);
    }

protected:
    const int          slotnum_;
    map<Slice, Entry*> dict_;
    Entry             *header_;
    Entry             *ender_;
};

};

#endif