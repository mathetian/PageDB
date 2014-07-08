// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _BUF_PAC_H
#define _BUF_PAC_H

#include "Slice.h"
#include "Atomic.h"
#include "Noncopyable.h"
using namespace utils;

#include "Log.h"

/**
** BufferPacket is the buffer container allocated in advance
**
** Users should fix the buffer size and append data into the container
** Users can discard all the content which have filled into the container anytime.
**
** BufferPacket is based on reference count,
** which means we don't need to care about the allocation and de-allocation.
**/
namespace pagedb
{

class BufferPacket
{
public:
    BufferPacket(int size);
    ~BufferPacket();
    BufferPacket(const BufferPacket & packet);
    BufferPacket & operator=(const BufferPacket & packet);

public:
    BufferPacket & operator << (int ivalue);
    BufferPacket & operator << (size_t st);
    BufferPacket & operator << (const string & str);
    BufferPacket & operator << (const Slice & slice);
    BufferPacket & operator << (const char * str);
    BufferPacket & operator << (const BufferPacket & packet);
    BufferPacket & operator << (const uint32_t value);

public:
    BufferPacket & operator >> (int    & ivalue);
    BufferPacket & operator >> (size_t & st);
    BufferPacket & operator >> (string & str);
    BufferPacket & operator >> (Slice  & slice);
    BufferPacket & operator >> (char * str);
    BufferPacket & operator >> (uint32_t & value);

public:
    void write(const char * str, int len);
    void read (char * str, int len);

public:
    /**
    ** c_str: return const char*, mostly used by write
    **/
    const char * c_str() const {
        return m_data;
    }
    /**
    ** str: return char*, mostly used by read
    **/
    char * str() const {
        return m_data;
    }

    int    size() const {
        return m_size;
    }

    int    getCursize() {
        return m_cur;
    }
    /**
    ** Sometime, we want to overwrite the content in the container
    ** In such time, we should set the position manually.
    **/
    void   reset() {
        m_cur = 0;
    }
    /**
    ** After we read something using str(), we need set cur position manually.
    **/
    void   setCurSize(int size) {
        assert(size <= m_size);
        m_cur = size;
    }

private:
    /**
    ** reference count
    **/
    void acquire();
    void release();

private:
    char *  m_data;
    int     m_size, m_cur;
    Log  *  m_log;
    Atomic *m_ref;

    /**
    ** Judge whether to same buffer pool & output summary of BufferPacket
    **/
    friend bool operator==(const BufferPacket &buff1, const BufferPacket &buff2);
    friend bool operator!=(const BufferPacket &buff1, const BufferPacket &buff2);
    friend ostream& operator<<(ostream &os, const BufferPacket &buff);
};

}

#endif