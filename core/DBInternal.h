// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _DB_INT_H
#define _DB_INT_H

#include "Log.h"
#include "Slice.h"
using namespace utils;

#include "Batch.h"

#define PAGESIZE 100
#define CACHESIZE 20

#define SINT     sizeof(int)
#define SINT64   sizeof(uint64_t)

/**
** DBInternal is the second layer of db.
** DBInternal is the base class of PageDB
**/
namespace customdb
{

typedef uint32_t (*HashFunc)(const Slice & key);

class DBInternal
{
public:
    DBInternal() : log(log::GetInstance())
    {
    }

public:
    /**
    ** Layer 1
    **/
    virtual bool   open(const char * filename) = 0;
    virtual bool   close();
    virtual bool   put(const Slice & key,const Slice & value) = 0;
    virtual Slice  get(const Slice & key) = 0;
    virtual bool   remove(const Slice & key) = 0;
    /**
    ** Layer 2
    **/
    /**
    ** Layer 3
    **/
    virtual void   dump() = 0;
    virtual void   removeDB(const char * filename) = 0;
    virtual void   fflush() = 0;
    virtual void   runBatch(const WriteBatch * pbatch) = 0;
    virtual void   write(WriteBatch* pbatch) = 0;
    virtual void   compact() = 0;
    virtual void   runBatchParallel(const WriteBatch * pbatch) = 0;

protected:
    Log  *  log;
};

};

#endif