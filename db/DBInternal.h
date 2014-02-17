#ifndef _FAC_H
#define _FAC_H

#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#include "../include/Log.h"
#include "../include/Slice.h"
#include "../include/Batch.h"

namespace customdb
{

#define PAGESIZE 100
#define CACHESIZE 20

#define SINT     sizeof(int)
#define SINT64   sizeof(uint64_t)

typedef uint32_t (*HASH)(const Slice & key);

class DBInternal
{
public:
    DBInternal()
    {
        log = Log::GetInstance();
    }
    virtual ~DBInternal() {}

public:
    /**Pure Virtual Function ??? why error, can't allocate enough spaces**/
    virtual bool   put(const Slice & key,const Slice & value) = 0;
    virtual Slice  get(const Slice & key) = 0;
    virtual bool   remove(const Slice & key) = 0;
    virtual bool   init(const char * filename) = 0;
    virtual void   dump() = 0;
    virtual void   removeAll(const char * filename) = 0;
    virtual void   fflush() = 0;
    virtual void   runBatch(const WriteBatch * pbatch) = 0;
    virtual void   write(WriteBatch* pbatch) = 0;
    virtual void   compact() = 0;
    virtual void   runBatch2(const WriteBatch * pbatch) = 0;

protected:
    Log  *  log;
};

};

#endif