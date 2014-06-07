// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _BATCH_H
#define _BATCH_H

#include "Slice.h"
using namespace utils;

/**
** Batch represents the archive of slices.
**/
namespace customdb
{

/**
** WriteBatch is the kernel class of batch
**/
class WriteBatch
{
private:
    typedef pair<Slice, Slice> Node;

public:
    WriteBatch(int size = 1000);

public:
    void     put(const Slice& key, const Slice& value);
    void     clear();
    uint32_t getTotalSize() const;
    int      getCount() const;

public:
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