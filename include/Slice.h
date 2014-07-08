// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _SLICE_H
#define _SLICE_H

#include "../utils/CommonHeader.h"

/**
** Slice is used to store immutable buffer.
**
** Slice behave as string, but it doesn't end with `\0`.
**/

namespace utils
{

class Slice
{
public:
    Slice();
    ~Slice();

public:
    /**
    ** initiated slice with length n
    **/
    Slice(size_t n);

    /**
    ** string copy, length n
    **/
    Slice(const char* d, size_t n);

    /**
    ** string copy
    **/
    Slice(const string& s);

    /**
    **
    **/
    Slice(const char*d);
    /**
    ** copy & operator construct
    **/
    Slice(const Slice &s1);
    Slice &operator=(const Slice & s1);

public:
    const char* c_str()    const;
    size_t      size()     const;

public:
    /**
    ** mostly, we shouldn't use `to_str()` because slice won't end with `\0`
    ** Only use it when you are sure that it is a string
    **/
    string      to_str()   const;
    /**
    ** Same as `to_str()`. Very limited situation.
    **/
    int     returnAsInt() const;

public:
    char   operator[](size_t n) const;
    char   at(size_t n) const;

private:
    const char * m_data;
    size_t       m_size;
};

extern bool operator==(const Slice & s1, const Slice & s2);
extern bool operator!=(const Slice & s1, const Slice & s2);
extern bool operator< (const Slice & s1, const Slice & s2);
extern bool operator> (const Slice & s1, const Slice & s2);
extern ostream & operator << (ostream & os, const Slice & sl);
extern istream & operator >> (istream & is, Slice & sl);

};

#endif