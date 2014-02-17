#include "MemoryPool.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

MemoryPool::MemoryPool(size_t initPoolSize, size_t eachChunkSize) : m_eachChunkSize(eachChunkSize)
{
    firstChunk = lastChunk = curChunk = NULL;
    m_totalMemoryPoolSize = 0;
    m_freeMemoryPoolSize  = 0;
    m_chunkNum = 0;
}

MemoryPool::~MemoryPool()
{
    freeAllMemory();
    freeAllChunks();
}

int MemoryPool::calculateNeededChunks(size_t size)
{
    float fnum = ((float)size)/((float)m_eachChunkSize);
    return (int)ceil(fnum);
}

MemoryChunk * MemoryPool::skipChunk(MemoryChunk * chunk,int step)
{
    while(step--) 
    {
        chunk = chunk -> nextChunk;
        if(!chunk) break;
    }
    return chunk;
}

MemoryChunk * MemoryPool::findSuitableChunks(int chunkNum)
{
    if(m_freeMemoryPoolSize <= chunkNum * m_eachChunkSize) return NULL;
    MemoryChunk * chunk = curChunk, * oldChunk = curChunk; int f = 1;
    while(chunk != oldChunk || f != 0)
    {   
        if(chunk -> totalChunk >= chunkNum)
        {
            if(chunk -> usedChunk == 0)
                return chunk;
        }
        int step = (chunk -> usedChunk != 0) ? (chunk -> usedChunk) : 1;
        chunk = skipChunk(chunk, step);
        if(chunk == NULL) chunk = firstChunk;
        f = 0;
    } 
    if(chunk == oldChunk) return NULL;
    return chunk;
}

bool MemoryPool::allocateMemory(size_t size)
{
    int needChunks = calculateNeededChunks(size);
    int needSize   = needChunks * m_eachChunkSize;
   
    char * memoryData    = new char[needSize];
    MemoryChunk * chunks = new MemoryChunk[needChunks];
    
    if(!memoryData || ! chunks)
    {
        if(memoryData) delete [] memoryData;
        if(chunks) delete [] chunks;
        memoryData = NULL; chunks = NULL;
        fprintf(stderr,"Error : allocateMemory error, maybe run out of memory\n.");
        return false;
    }    

    m_totalMemoryPoolSize += needSize;
    m_freeMemoryPoolSize  += needSize;
    
    memset(chunks,0xff,sizeof(MemoryChunk) * needChunks);

    int offset = 0, index = 0;
    for(;index < needChunks;index++)
    {
        if(firstChunk == NULL)
        {
            firstChunk = chunks;
            curChunk = lastChunk = chunks;
        }
        else
        {  
            lastChunk -> nextChunk = \
                chunks + index;
            lastChunk = chunks + index;
        }
        chunks[index].totalChunk = needChunks - index;
        chunks[index].usedChunk  = 0;
        chunks[index].data       = memoryData + offset;
        if(index == 0) chunks[index].header = true;
        offset += m_eachChunkSize;
    }
    return true;
}

char * MemoryPool::getMemory(size_t size)
{
    int needChunks = calculateNeededChunks(size);
    int needSize   = needChunks * m_eachChunkSize;
    MemoryChunk * chunk = NULL;
    while(chunk == NULL)
    {
        chunk = findSuitableChunks(needChunks);
        if(chunk == NULL)
            allocateMemory(needSize);
    }
    m_freeMemoryPoolSize -= needSize;
    chunk -> usedChunk    = needChunks;
    return chunk -> data;
}

bool MemoryPool::freeMemory(char * data, size_t size)
{
    int needChunks = calculateNeededChunks(size);
    int needSize   = needChunks * m_eachChunkSize;
    MemoryChunk * chunk = firstChunk;
    while(chunk && chunk -> data != data)
        chunk =  chunk -> nextChunk;
    
    if(!chunk || chunk -> usedChunk != needChunks) 
        return false;
    
    if(chunk -> usedChunk == needChunks)
        chunk -> usedChunk = 0;
    
    return true;
}

void MemoryPool::freeAllMemory()
{
    MemoryChunk * chunk = firstChunk;
    
    while(chunk)
    {
        if(chunk -> header == true)
            delete [] (chunk -> data);
        chunk -> data = NULL;
        chunk = chunk -> nextChunk;
    }
}

void MemoryPool::freeAllChunks()
{
    MemoryChunk * dleChunk = NULL;
    MemoryChunk * curChunk = firstChunk;
    
    while(curChunk)
    {
        if(curChunk -> header == true)
        {
            if(dleChunk)
                delete [] dleChunk;
            dleChunk = curChunk;
        }
        curChunk = curChunk -> nextChunk;
    }

    if(dleChunk) 
        delete [] dleChunk;
    dleChunk = NULL;
}