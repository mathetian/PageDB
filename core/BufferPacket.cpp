// Copyright (c) 2014 The PageDB1 Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "BufferPacket.h"

namespace pagedb
{

void BufferPacket::acquire()
{
    if (m_ref) ++*m_ref;
}

void BufferPacket::release()
{
    if (m_ref && (m_ref->addAndGet(-1)) == 0 && m_data)
    {
        delete [] m_data;
        delete m_ref;
        m_data = NULL;
        m_ref = NULL;
    }
}

BufferPacket::BufferPacket(int size) : m_size(size), m_cur(0), m_ref(new Atomic(0))
{
    m_data    = new char[m_size];
    memset(m_data, 0xff, m_size);

    m_log    = Log::GetInstance();

    acquire();
}

BufferPacket::~BufferPacket()
{
    release();
}

BufferPacket::BufferPacket(const BufferPacket & packet) : m_ref(packet.m_ref)
{
    m_data = packet.m_data;
    m_size = packet.m_size;
    m_cur  = packet.m_cur;

    acquire();
}

BufferPacket & BufferPacket::operator=(const BufferPacket & packet)
{
    release();

    m_data = packet.m_data;
    m_size = packet.m_size;
    m_cur  = packet.m_cur;

    m_ref = packet.m_ref;
    acquire();

    return *this;
}

BufferPacket & BufferPacket::operator << (int ivalue)
{
    if(m_cur + sizeof(int) > m_size)
    {
        m_log -> _Error("BufferPacket error << int\n");
        assert(0);
    }
    else
    {
        *(int*)(m_data + m_cur) = ivalue;
        m_cur += sizeof(int);
    }

    return *this;
}

BufferPacket & BufferPacket::operator << (const uint32_t ivalue)
{
    if(m_cur + sizeof(uint32_t) > m_size)
    {
        m_log -> _Error("BufferPacket error << int\n");
        assert(0);
    }
    else
    {
        *(uint32_t*)(m_data + m_cur) = ivalue;
        m_cur += sizeof(uint32_t);
    }

    return *this;
}

BufferPacket & BufferPacket::operator << (size_t st)
{
    if(m_cur + sizeof(size_t) > m_size)
    {
        m_log -> _Error("BufferPacket error << size_t\n");
        assert(0);
    }
    else
    {
        *(size_t*)(m_data + m_cur) = st;
        m_cur += sizeof(size_t);
    }
    return *this;
}

BufferPacket & BufferPacket::operator << (const string & str)
{
    if(m_cur + str.size() > m_size)
    {
        m_log -> _Error("BufferPacket error << string\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur, str.c_str(), str.size());
        m_cur += str.size();
    }
    return *this;
}


BufferPacket & BufferPacket::operator >> (int & ivalue)
{
    if(m_cur + sizeof(ivalue) > m_size)
    {
        m_log -> _Error("BufferPacket >> int overflow\n");
        assert(9);
    }
    else
    {
        ivalue = *(int*)(m_data + m_cur);
        m_cur += sizeof(ivalue);
    }
    return *this;
}

BufferPacket & BufferPacket::operator >> (uint32_t & ivalue)
{
    if(m_cur + sizeof(ivalue) > m_size)
    {
        m_log -> _Error("BufferPacket >> int overflow\n");
        assert(0);
    }
    else
    {
        ivalue = *(uint32_t*)(m_data + m_cur);
        m_cur += sizeof(uint32_t);
    }
    return *this;
}

BufferPacket & BufferPacket::operator >> (size_t & st)
{
    if(m_cur + sizeof(st) > m_size)
    {
        m_log -> _Error("BufferPacket >> size_t overflow\n");
        assert(0);
    }
    else
    {
        st = *(size_t*)(st + m_cur);
        m_cur += sizeof(st);
    }
    return *this;
}

BufferPacket & BufferPacket::operator >> (string & str)
{
    if(m_cur + str.size() > m_size)
    {
        m_log -> _Error("BufferPacket >> string overflow\n");
        assert(0);
    }
    else
    {
        int index = 0;
        for(; index < str.size(); index++)
            str[index] = m_data[m_cur + index];
        m_cur += str.size();
    }
    return *this;
}

void BufferPacket::write(const char * str, int len)
{
    if(m_cur + len > m_size)
    {
        m_log -> _Error("BufferPacket error write\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur, str, len);
        m_cur += len;
    }
}


void BufferPacket::read(char * str, int len)
{
    if(m_cur + len > m_size)
    {
        m_log -> _Error("BufferPacket >> char* overflow\n");
        assert(0);
    }
    else
    {
        memcpy(str,m_data + m_cur,len);
        m_cur += len;
    }
}

BufferPacket & BufferPacket::operator << (const BufferPacket & packet)
{
    if(m_cur + packet.size() > m_size)
    {
        m_log -> _Error("BufferPacket write packet overflow\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur,(char*)&packet,packet.size());
        m_cur += packet.size();
    }

    return *this;
}

BufferPacket & BufferPacket::operator << (const char * str)
{
    if(m_cur + strlen(str) > m_size)
    {
        m_log -> _Error("BufferPacket write char * overflow\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur, str, strlen(str));
        m_cur += strlen(str);
    }

    return *this;
}

BufferPacket & BufferPacket::operator << (const Slice & slice)
{
    if(m_cur + slice.size() > m_size)
    {
        m_log -> _Error("BufferPacket write slice overflow\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur, slice.c_str(), slice.size());
        m_cur += slice.size();
    }

    return *this;
}

BufferPacket & BufferPacket::operator >> (Slice & slice)
{
    if(m_cur + slice.size() > m_size)
    {
        m_log -> _Error("BufferPacket >> slice overflow\n");
        assert(0);
    }
    else
    {
        Slice rs(m_data + m_cur, slice.size());
        slice = rs;
        m_cur += slice.size();
    }

    return *this;
}

BufferPacket & BufferPacket::operator >> (char * str)
{
    int index = m_cur;
    while(index < m_size)
    {
        if(m_data[index] == 0) break;
        index++;
    }

    memcpy(str, m_data + m_cur, index - m_cur + 1);

    m_cur = index + 1;

    return *this;
}

bool operator == (const BufferPacket &buff1, const BufferPacket &buff2)
{
    return (buff1.m_size == buff2.m_size && buff1.m_data == buff2.m_data) ? true : false;
}

bool operator != (const BufferPacket &buff1, const BufferPacket &buff2)
{
    return buff1 == buff2 ? false : true;
}

ostream& operator<<(ostream &os, const BufferPacket &buff)
{
    os <<  "[" << &buff.m_data << " : " << buff.m_size << endl;

    return os;
}

};