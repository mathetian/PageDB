// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Slice.h"

namespace utils
{

Slice::Slice() : m_data(NULL), m_size(0) { }

Slice::Slice(size_t n) : m_size(n)
{
    if(n == 0)
        m_data = NULL;
    else
    {
        char * data = new char[m_size];
        memset(data, 0, m_size);
        m_data = data;
    }
}

Slice::Slice(const char* d, size_t n) : m_size(n)
{
    char * data =  new char[m_size];
    memset(data, 0, m_size);
    memcpy(data, d, m_size);

    m_data = data;
}

Slice::Slice(const string& s)
{
    m_size = s.size();

    char * data =  new char[m_size];
    memset(data, 0, m_size);
    memcpy(data, s.data(), m_size);

    m_data = data;
}

Slice::Slice(const char *d)
{
    m_size = strlen(d);

    char * data =  new char[m_size];
    memset(data, 0, m_size);
    memcpy(data, d, m_size);

    m_data = data;
}

Slice::Slice(const Slice & s1) : m_size(s1.m_size), m_data(NULL)
{
    if(m_size != 0)
    {
        char * data = new char[m_size];
        memcpy(data, s1.m_data, m_size);

        m_data = data;
    }
}

Slice & Slice::operator=(const Slice & s1)
{
    if(m_data != NULL && s1.m_data != m_data)
    {
        delete [] m_data;
        m_data = NULL;
        m_size = 0;
    }
    else if(s1.m_data == m_data)
    {
        return *this;
    }

    m_size = s1.m_size;

    if(m_size != 0)
    {
        char * data = new char[m_size];
        memcpy(data, s1.m_data, m_size);

        m_data = data;
    }

    return *this;
}

Slice::~Slice()
{
    if(m_data != NULL)
        delete [] m_data;

    m_size = 0;
    m_data = NULL;
}

const char* Slice::c_str() const
{
    return m_data;
}

/**
** Notice
**/
string Slice::to_str() const
{
    return string(m_data, m_size);
}

int  Slice::returnAsInt() const
{
    int r = 0;
    for(unsigned int i = 0; i < m_size; i++)
    {
        int a = m_data[i];

        for(unsigned int j = 0; j < 8; j++)
        {
            int flag = ((a & (1 << j)) >> j);
            r += (flag << (i*8 + j));
        }
    }
    return r;
}

size_t Slice::size() const
{
    return m_size;
}

char Slice::operator[](size_t n) const
{
    assert(n < size());
    return m_data[n];
}

char Slice::at(size_t n) const
{
    return operator[](n);
}

bool operator == (const Slice & s1, const Slice & s2)
{
    if(s1.size() != s2.size()) return false;

    unsigned i = 0;

    while(i < s1.size() && s1.at(i) == s2.at(i)) i++;

    if(i == s1.size()) return true;

    return false;
}

bool operator != (const Slice & s1, const Slice & s2)
{
    return !(s1 == s2);
}

bool operator < (const Slice & s1, const Slice & s2)
{
    unsigned int i = 0;
    while(i < s1.size() && i < s2.size() && s1[i] == s2[i])
        i++;

    if(i == s2.size()) return false;
    else if(i == s1.size()) return true;
    else if(s1[i] < s2[i]) return true;
    else return false;
}

bool operator > (const Slice & s1, const Slice & s2)
{
    return s2 < s1;
}

ostream & operator << (ostream & os, const Slice & slice)
{
    os << "Slice(" << slice.to_str() << ")";

    return os;
}

istream & operator >> (istream & is, Slice&slice)
{
    string str;
    is >> str;
    slice = Slice(str);

    return is;
}

};