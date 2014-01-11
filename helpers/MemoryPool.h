#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include <string.h>

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

class ScopeMemory{
public: 
	ScopeMemory(uint32_t size = 4096)
	{
		m_nMemSize = size;
		m_pMem = new char[m_nMemSize];
	}
	~ ScopeMemory()
	{
		if(m_pMem) delete [] m_pMem;
		m_pMem = NULL;
	}
public:
	char * operator()(int x = 1) { return m_pMem; }
	char * getData() { return m_pMem;}
private:
	char     * m_pMem;
	uint32_t   m_nMemSize;
};

class StaticMemory{
public:
	StaticMemory * getInstance()
	{
		if(m_sMem == NULL)
			m_sMem = new StaticMemory;
		return m_sMem;
	}
	~ StaticMemory() {}
public:
    char * allocate(uint32_t nNeedSize)
    {
    	if(nNeedSize <= m_nMemMax)
    		return m_pMem;
    	if(m_pMem != NULL)
    	{
    		delete m_pMem;
    		m_pMem    = NULL;
    		m_nMemMax = 0;
    	}
    	m_nMemMax = 2*nNeedSize;
    	if((m_pMem = new char[m_nMemMax]) == NULL)
    		m_nMemMax = 0;
    	return m_pMem;
    }

    void release()
    {
    	if(m_pMem != NULL)
    	{
    		delete m_pMem;
    		m_pMem    = NULL;
    		m_nMemMax = 0;
    	}
    }
private:
	StaticMemory() { m_pMem = NULL; m_nMemMax = 0; }
	StaticMemory * m_sMem;
    char         * m_pMem;
    uint32_t       m_nMemMax;
};

class MemoryPool;

class MemoryChunk{
public:
    MemoryChunk()
    { 
    	data       = NULL; 
    	nextChunk  = NULL; 
    	totalChunk = usedChunk = 0;
    	header     = false;
    }
  ~ MemoryChunk()
    {
	  	if(data != NULL) 
	  		delete [] data;
	  	data = NULL; nextChunk = NULL;
    }
private:
	char  * data;
	int     totalChunk;
	int     usedChunk;
	MemoryChunk * nextChunk;
	bool    header;
	friend class MemoryPool;
};

class MemoryPool{
public:
	MemoryPool(size_t initPoolSize  = 100, 
			   size_t eachChunkSize = 10);
	~ MemoryPool();
public:
	char * getMemory(size_t size);
	bool   freeMemory(char * p, size_t size);
private:
	bool   allocateMemory(size_t size);
	void   freeAllMemory();
	void   freeAllChunks();
	int    calculateNeededChunks(size_t size);
private:
	MemoryChunk * findSuitableChunks(int chunkNUm);
	MemoryChunk * skipChunk(MemoryChunk * ptrCur, int step);
private:
	MemoryChunk * firstChunk;
	MemoryChunk * lastChunk;
	MemoryChunk * curChunk;
	size_t        m_totalMemoryPoolSize;
	size_t        m_freeMemoryPoolSize;
	const size_t        m_eachChunkSize;
	int           m_chunkNum;
};

#endif