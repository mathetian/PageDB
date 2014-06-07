// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _ATOMOC_H
#define _ATOMOC_H

/**
** Atomic is used to represent some atomic variable.
** Detail information for it can be found in
** gcc.gnu.org/onlinedocs/libstdc++/manual/ext_concurrency.html
**/

namespace utils
{

/**
    Helper function
**/
static inline int  __exchange_and_add (volatile int *__mem, int __val)
{
    register int __result;
    __asm__ __volatile__ ("lock; xaddl %0,%2"
                          : "=r" (__result)
                          : "0" (__val), "m" (*__mem)
                          : "memory");
    return __result;
}

class Atomic
{
    int val;
public:
    Atomic(int val = 0) : val(val) { }

    /**CAS: compare and swap**/
    int exchange_and_add(int addend)
    {
        return __exchange_and_add(&val, addend);
    }

    void add(int addend)
    {
        __sync_add_and_fetch(&val, addend);
    }

    int addAndGet(int addend)
    {
        return __sync_add_and_fetch(&val, addend);
    }

    void operator += (int addend)
    {
        add(addend);
    }

    void operator -= (int addend)
    {
        add(-addend);
    }

    /**Prefix add one**/
    void operator ++ ()
    {
        add(1);
    }

    /**Prefix minus one**/
    void operator -- ()
    {
        add(-1);
    }

    /**Posix add one**/
    int operator ++ (int)
    {
        return exchange_and_add(1);
    }

    /**Posix minus one**/
    int operator -- (int)
    {
        return exchange_and_add(-1);
    }

    operator int() const
    {
        return val;
    }
};

}
#endif
