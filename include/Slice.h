#ifndef _SLICE_H
#define _SLICE_H

#include <iostream>
#include <string>
using namespace std;

namespace customdb
{

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
    /**Just for debug**/
    void    printAsInt()  const;
    int     returnAsInt() const;

public:
    char   operator[](size_t n) const;
    Slice& operator+=(const Slice&s1);
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