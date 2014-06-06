#ifndef _SLICE_H
#define _SLICE_H

#include <iostream>
#include <string>
using namespace std;

#include "Noncopyable.h"

namespace utils
{

/**
** Slice is used to store immutable buffer.
**/

class Slice
{
public:
    Slice();
    ~Slice();

public:
    Slice(size_t n);
    Slice(const char* d, size_t n);
    Slice(const string& s);
    Slice(const char* s);
    Slice(const Slice &s1);
    Slice &operator=(const Slice & s1);
    
public:
    const char* tochars()  const;
    const char* c_str()    const;
    string      toString() const;
    size_t      size()     const;
    bool        empty()    const;
    void        clear();

public:
    /**
    ** For Debug
    ** Must not be used in production environment
    **/
    void    printAsInt()  const;
    int     returnAsInt() const;

public:
    char   operator[](size_t n) const;
    operator string();

private:
    const char * m_data;
    size_t       m_size;
};

extern bool operator==(const Slice & s1, const Slice & s2);
extern bool operator!=(const Slice & s1, const Slice & s2);
extern bool operator< (const Slice & s1, const Slice & s2);
extern bool operator> (const Slice & s1, const Slice & s2);
extern ostream & operator << (ostream & os, const Slice & sl);
extern istream & operator >> (istream & is, Slice&sl);

};

#endif