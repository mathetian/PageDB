#include "BufferPacket.h"

using namespace customdb;

BufferPacket::BufferPacket(int size) : m_size(size), m_cur(0)
{
    m_data    = new char[m_size];
    memset(m_data, 0xff, m_size);

    m_log    = Log::GetInstance();
}

BufferPacket::~BufferPacket()
{
    if(m_data)
    {
        delete [] m_data;
        m_data = NULL;
    }
    m_size = m_cur = 0;
}

BufferPacket::BufferPacket(const BufferPacket & packet)
{
    m_data = new char[packet.m_size];
    memcpy(m_data, packet.m_data, packet.m_size);

    m_size = packet.m_size;
    m_cur  = packet.m_cur;
}

BufferPacket & BufferPacket::operator=(const BufferPacket & packet)
{
    if(m_size != 0) delete [] m_data;

    m_data = new char[packet.m_size];
    memcpy(m_data, packet.m_data, packet.m_size);

    m_size = packet.m_size;
    m_cur  = packet.m_cur;

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

BufferPacket & BufferPacket::operator<<(const BufferPacket & packet)
{
    if(m_cur + packet.getSize() > m_size)
    {
        m_log -> _Error("BufferPacket write packet overflow\n");
        assert(0);
    }
    else
    {
        memcpy(m_data + m_cur,(char*)&packet,packet.getSize());
        m_cur += packet.getSize();
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
        memcpy(m_data + m_cur, slice.tochars(), slice.size());
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