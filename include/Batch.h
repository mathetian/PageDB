// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _BATCH_H
#define _BATCH_H

#include "Slice.h"
using namespace utils;

/**
** Batch represents the archive of slices.
**/
namespace pagedb
{

/**
** WriteBatch is the kernel class of batch
**/
class WriteBatch
{
private:
    typedef pair<Slice, Slice> Node;

public:
    /**
    ** To avoid useless duplicate of Node, we pre-allocated a buffer
    **/
    WriteBatch(int size = 1000);

public:
    void     put(const Slice& key, const Slice& value);
    void     clear();
    /**
    ** Get size of batch
    **/
    uint32_t getTotalsize() const;
    /**
    ** Get count of items
    **/
    int      getCount() const;

public:
    /**
    ** Inner Class, Iterator: Can be used as STL's iterator
    **/
    class Iterator
    {
    public:
        Iterator(const WriteBatch * pbatch);
        const Node * next();
        const Node * prev();
        void seekToFirst();
        void seekToEnd();
        const Node * first();
        const Node * end();
    private:
        const WriteBatch* m_pbatch;
        int m_curNum;
    };

private:
    vector<Node> m_ssvec;
    int          m_size;
    int          m_num;
    uint32_t     m_msize;
};

/**
** XXXInternel represents some helper function for batch
**/
class WriteBatchInternal
{
public:
    typedef pair<Slice, Slice> Node;

public:
    static void Append(WriteBatch * dst, const WriteBatch * src);
    static uint32_t ByteSize(WriteBatch * dst);
    static uint32_t Count(WriteBatch * dst);
};

};
#endif