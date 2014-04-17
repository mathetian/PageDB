#ifndef _ATOMOC_H
#define _ATOMOC_H

namespace utils
{

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
    Atomic(int val = 0) : val(val) {}

    inline int exchange_and_add(int addend)
    {
        return __exchange_and_add(&val, addend);
    }

    inline void add(int addend)
    {
        __sync_add_and_fetch(&val, addend);
    }

    inline int addAndGet(int addend)
    {
        return __sync_add_and_fetch(&val, addend);
    }

    inline void operator += (int addend)
    {
        add(addend);
    }

    inline void operator -= (int addend)
    {
        add(-addend);
    }

    inline void operator ++ ()
    {
        add(1);
    }

    inline void operator -- ()
    {
        add(-1);
    }

    inline int operator ++ (int)
    {
        return exchange_and_add(1);
    }

    inline int operator -- (int)
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
