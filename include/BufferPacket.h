#ifndef _BUF_PAC_H
#define _BUF_PAC_H

#include <string>
using namespace std;

#include <stdint.h>
#include <string.h>

#include "Slice.h"
#include "Atomic.h"
using namespace utils;

#include "Log.h"

namespace customdb
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
    char * getData()
    {
        return m_data;
    }
    int    getSize() const
    {
        return m_size;
    }
    void   setBeg()
    {
        m_cur = 0;
    }

private:
    void acquire();
    void release();

private:
    char *  m_data;
    int     m_size, m_cur;
    Log  *  m_log;
    Atomic *m_ref;

};

}

#endif