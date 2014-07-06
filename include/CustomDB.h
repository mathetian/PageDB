// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "Log.h"
#include "Slice.h"
using namespace utils;

#include "Cache.h"
using namespace cache;

#include "Batch.h"
#include "Option.h"
#include "DBInternal.h"

/**
** The first layer of DB.
**
** Basic operations:
** `open/close/put/get/remove`
**
** Improved operations:
** `put(batch)`
** `write(batch)`
** `runBatchParallel(batch)`
**
** Other operations:
** `sync/dump/compact/destoryDB/checkStatus`
**
**/
namespace customdb
{

#define ERROR 0
#define SUCCE 1

class CustomDB
{
public:
    CustomDB();
    virtual ~CustomDB();

public:
    /**
    ** level 1, basic operations
    **/
    bool   open(const Options & option);
    void   close();
    bool   put(const Slice & key,const Slice & value);
    Slice  get(const Slice & key);
    bool   remove(const Slice & key);

public:
    /**
    ** level 2, improved operations
    **/
    bool   put(WriteBatch * pbatch);

public:
    /**
    ** level 3, other operations
    **/
    void   sync();
    void   dump(ostream&os = cout);
    void   compact();
    void   destoryDB(const char * filename);
    int    checkStatus();

private:
    Options   	m_option;
    DBInternal *m_dbimpl;
    BaseCache  *m_cache;
    Log        *m_log;
    /**
    ** Only record the lastest status
    **/
    int         m_lastStatus;
};

};
#endif