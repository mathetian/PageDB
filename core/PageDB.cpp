// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDB.h"

#include "FileModule.h"
using namespace utils;

/**
** Cache Header
**/
#include "FIFOCache.h"
#include "LRUCache.h"
#include "EmptyCache.h"
using namespace cache;

#include "PageDBImpl.h"

namespace pagedb
{

PageDB::PageDB() : m_dbimpl(NULL), m_cache(NULL), m_log(NULL) { }

PageDB::~PageDB()
{
    close();
}

/**
** level 1, Basic operations
**/

bool  PageDB::open(const Options & option)
{
    m_option = option;


    m_log -> SetLogInfo(option.logOption.logLevel, option.logOption.logPrefix, option.logOption.disabled);

    if(option.cacheOption.disabled == true)
        m_cache = new EmptyCache();
    else
        switch(option.cacheOption.cacheType)
        {
        case FIFO:
            m_cache = new FIFOCache(option.cacheOption.slotNum);
            break;
        case LRU:
            m_cache = new LRUCache(option.cacheOption.slotNum);
            break;
        case EMPTY:
            m_cache = new EmptyCache();
            break;
        default:
            m_log -> _Fatal("PageDB::open::cacheType error\n");
            break;
        }

    if(m_cache == NULL)
        m_log -> _Fatal("PageDB::open::new cache error\n");

    switch(option.factoryOption.factoryType)
    {
    case EHASH:
        m_dbimpl = new PageDB();
        break;
    case CHASH:
        break;
    default:
        m_log -> _Fatal("PageDB::open::dbimpl error\n");
    }

    if(m_dbimpl == NULL)
        m_log -> _Fatal("PageDB::open::new dbimpl error\n");

    if(m_dbimpl -> open(option.fileOption.fileName) == false)
        m_log -> _Fatal("PageDB::open::init dbimpl error\n");

    m_log -> _Trace("PageDB::open initialization successfully\n");
    return true;
}

void   PageDB::close()
{
    if(m_dbimpl)  m_dbimpl -> sync();

    if(m_dbimpl)  delete m_dbimpl;
    if(m_cache)   delete m_cache;

    m_dbimpl = NULL;
    m_cache  = NULL;
}

bool PageDB::put(const Slice & key,const Slice & value)
{
    assert(value.size() != 0 && key.size() != 0);

    m_lastStatus = ERROR;

    Slice cacheValue = m_cache -> get(key);

    if(cacheValue.size() != 0 && cacheValue == value)
        m_log -> _Warn("PageDB::put::exist in cache\n");
    else
    {
        m_log -> _Debug("PageDB::put::not exist in cache\n");

        if(m_dbimpl -> put(key, value) == false)
            m_log -> _Warn("PageDB::put::dbimpl put error\n");
        else
        {
            m_log -> _Debug("PageDB::put::dbimpl put successfully\n");

            m_cache -> put(key,value);
            m_lastStatus = SUCCE;
        }
    }

    return m_lastStatus;
}

Slice PageDB::get(const Slice & key)
{
    m_lastStatus = ERROR;

    Slice cacheValue = m_cache -> get(key);

    if(cacheValue.size() != 0)
        m_lastStatus = SUCCE;
    else
    {
        m_log -> _Debug("PageDB::get::dbimpl not in cache\n");
        cacheValue = m_dbimpl -> get(key);
        if(cacheValue.size() == 0)
            m_log -> _Warn("PageDB::get::dbimpl get warning\n");
        else
        {
            m_log -> _Debug("PageDB::get::dbimpl get successfully\n");

            m_cache -> put(key, cacheValue);
            m_lastStatus = SUCCE;
        }
    }

    return cacheValue;
}

bool PageDB::remove(const Slice & key)
{
    m_lastStatus = ERROR;

    Slice cacheValue = m_cache -> get(key);

    if(cacheValue.size() != 0)
    {
        if((m_cache -> remove(key)) == 0)
        {
            m_log -> _Error("PageDB::remove::cache remove error\n");
            return m_lastStatus;
        }
        else m_log -> _Trace("PageDB::remove::cache remove successfully\n");
    }

    if((m_dbimpl -> remove(key)) == 0)
        m_log -> _Error("PageDB::remove::dbimpl remove error\n");

    m_lastStatus = SUCCE;
    return m_lastStatus;
}

/**
** level 2, Improved operations
**/

bool PageDB::put(WriteBatch * pbatch)
{
    return m_dbimpl -> put(pbatch);
}

/**
** level 3, Other operations
**/

void PageDB::sync()
{
    m_dbimpl -> sync();
}

void   PageDB::dump(ostream&os)
{
    m_dbimpl -> dump(os);
}

void  PageDB::compact()
{
    m_dbimpl -> compact();
}

void   PageDB::destoryDB(const char * filename)
{
    string s_filename(filename,
                      filename + strlen(filename));

    string idxName = s_filename + ".idx";
    string datName = s_filename +  ".dat";

    FileModule::Remove(idxName);
    FileModule::Remove(datName);
}

int PageDB::checkStatus()
{
    return m_lastStatus;
}

};