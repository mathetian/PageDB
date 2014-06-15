// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace customdb
{

PageCache::PageCache(PageDB * db) : m_cur(0), m_db(db)
{
}

PageCache::~PageCache()
{
    free();
}

void    PageCache::free()
{
    sync();
    for(int i = 0; i < CACHESIZE; i++)
        m_eles[i].reset();
}

PageTable * PageCache::find(uint32_t addr, uint32_t & index)
{
    for(int i = 0; i < CACHESIZE; i++)
    {
        if(m_eles[i].m_addr == addr)
        {
            index = i;
            m_cur = (i+1)%CACHESIZE;
            return m_eles[i].m_page;
        }
    }
    return NULL;
}


int PageCache::put(PageTable * page, int addr)
{
    int i = (m_cur+1)%CACHESIZE;
    for(; i != m_cur; i = (i+1)%CACHESIZE)
    {
        if(m_eles[i].m_updated == false)
        {
            m_eles[i].reset();

            m_eles[i].m_page  = page;
            m_eles[i].m_addr = addr;
            break;
        }
    }

    int oldcur = i;

    if(i == m_cur)
    {
        if(m_eles[i].m_updated == true)
            reset(i);

        m_eles[i].reset();

        m_eles[i].m_page = page;
        m_eles[i].m_addr = addr;
    }

    m_cur = (i+1)%CACHESIZE;

    return oldcur;
}

void   PageCache::updated(int index)
{
    m_eles[index].m_updated = true;
}

void   PageCache::reset(int index)
{
    PageTable * page = m_eles[index].m_page;
    assert(page != NULL);
    BufferPacket packet = page -> getPacket();

    m_db -> m_datfile.Write(packet.c_str(), m_eles[index].m_addr, packet.size());
}


void   PageCache::sync()
{
    for(int i = 0; i < CACHESIZE; i++)
    {
        if(m_eles[i].m_updated == true)
        {
            PageTable * page = m_eles[i].m_page;
            BufferPacket packet = page -> getPacket();
            m_db -> m_datfile.Write(packet.c_str(), m_eles[i].m_addr, packet.size());
            m_eles[i].m_updated = false;
        }
    }
    m_cur = 0;
}

};

