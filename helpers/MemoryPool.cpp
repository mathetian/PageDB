#include "MemoryPool.h"
#include <stddef.h>
#include <sys/sysinfo.h>

uint64_t GetAvailPhysMemorySize()
{   
    struct sysinfo info;
    sysinfo(&info);
    return info.freeram;
}

MemoryPool::MemoryPool()
{
    m_pMem     = NULL;
    m_nMemSize = 0;
    m_nAvailPhys = GetAvailPhysMemorySize();

    if(m_nAvailPhys < 16*24*24)
        m_nMemMax = m_nAvailPhys >> 1;
    else
        m_nMemMax = m_nAvailPhys - 8*1024*1024;
}

MemoryPool::~MemoryPool()
{
    if(m_pMem)
    {
        delete m_pMem;
        m_pMem = NULL;
    }
}

unsigned char* MemoryPool::Allocate(uint64_t nFileLen, uint64_t & nAllocatedSize)
{
    unsigned int nTargetSize;

    if(nFileLen <= m_nMemSize)
    {
        nAllocatedSize = nFileLen;
        return m_pMem;
    }

    nTargetSize = (nFileLen < m_nMemMax) ? nFileLen : m_nMemMax;

    if(m_pMem != NULL)
    {
        delete m_pMem;
        m_pMem = NULL;
        m_nMemSize = 0;
    }

    m_pMem = new unsigned char[nTargetSize];

    if(m_pMem != NULL)
    {
        m_nMemSize     = nTargetSize;
        nAllocatedSize = nTargetSize;
        return m_pMem;
    }
    else
    {
        nAllocatedSize = 0;
        return NULL;
    }
}