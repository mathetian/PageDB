#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

typedef unsigned long long uint64_t;

class MemoryPool
{
public:
    MemoryPool();
    virtual ~ MemoryPool();

public:
    char*Allocate(uint64_t nFileLen,uint64_t&nAllocateSize);

private:
    char      * m_pMem;
    uint64_t    m_nMemSize;
    uint64_t    m_nMemMax;
    uint64_t    m_nAvailPhys;
};

#endif