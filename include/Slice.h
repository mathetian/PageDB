#ifndef _SLICE_H
#define _SLICE_H

#include <string>
#include <iostream>
using namespace std;

#include <assert.h>
#include <stddef.h>
#include <string.h>

class Slice
{
public:
    Slice() : m_data(""), m_size(0) { }
    
    Slice(size_t n) : m_size(n)
    {
        char * data = new char[m_size];
        memset(data, 0, m_size);
        m_data = data;
    }
    
    Slice(const char* d, size_t n) : m_size(n)
    {
        char * data =  new char[m_size];
        memcpy(data, d, m_size);
        m_data = data;
    }

    Slice(const string& s) : m_data(s.data()), m_size(s.size()) {}

    Slice(const char* s) : m_size(strlen(s))
    {
        char * data = new char[m_size + 1];
        memset(data, 0, m_size + 1);
        memcpy(data, s, m_size + 1);

        m_data = data;
    }

    Slice(const Slice & s1) : m_size(s1.m_size)
    {
        char * data = new char[m_size];
        memcpy(data, s1.m_data, m_size);
        m_data = data;
    }

    Slice & operator=(const Slice & s1)
    {
        this -> m_size = s1.m_size;
        char * data = new char[m_size];
        memcpy(data, s1.m_data, m_size);
        this -> m_data = data;
        return *this;
    }
    
    ~Slice()
    {
        if(m_size != 0) delete [] m_data;
        m_size = 0; m_data = NULL;
    }
public:
    const char* tochars()  const
    {
        return m_data;
    }

    const char* c_str() const
    {
        return m_data;
    }
    
    string toString() const
    {
        return string(m_data, m_size);
    }

    size_t size() const
    {
        return m_size;
    }

    bool  empty() const
    {
        return m_size == 0;
    }

    void clear()
    {
        m_data = "";
        m_size = 0;
    }

public:
    char operator[](size_t n) const
    {
        assert(n < size());
        return m_data[n];
    }

    Slice& operator+=(const Slice&s1);

private:
    const char * m_data;
    size_t       m_size;
};

inline bool operator==(const Slice & s1, const Slice & s2)
{
    if(s1.size() != s2.size()) return false;
    
    int i = 0;
    
    while(i < s1.size() && s1[i] == s2[i]) i++;
  
    if(i == s1.size()) return true;
    
    return false;
}

inline bool operator< (const Slice & s1, const Slice & s2)
{
    int i = 0;
    while(i < s1.size() && i < s2.size() && s1[i] == s2[i]) 
        i++;

    if(i == s2.size()) return false;
    else if(i == s1.size()) return true;
    else if(s1[i] < s2[i]) return true;
    else return false;    
}

inline bool operator> (const Slice & s1, const Slice & s2)
{
    return s2 < s1;
}

inline ostream & operator << (ostream & os, const Slice & sl)
{
    os << sl.toString();
    return os;
}

inline istream & operator >> (istream & is, Slice&sl)
{
    string str;
    is >> str;
    sl = Slice(str);
    return is;
}

#endif