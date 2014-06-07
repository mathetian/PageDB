// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _SLICE_H
#define _SLICE_H

#include "CommonHeader.h"

/**
** Slice is used to store immutable buffer.
**/

namespace utils
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
    /**
    ** For Debug
    ** Can't be used in production environment
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