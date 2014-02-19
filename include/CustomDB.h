#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "Log.h"
#include "Batch.h"
#include "Slice.h"
#include "Option.h"
#include "BaseCache.h"
#include "DBInternal.h"

#include <string>
using namespace std;

namespace customdb
{

#define ERROR 0
#define SUCCE 1

class CustomDB
{
public:
    CustomDB();
    virtual ~CustomDB();

public:
    void   close();
    void   dump();
    void   cleanCACHE();
    void   destoryDB(const char * filename);
    void   fflush();

public:
    bool   open(const Options & option);
    bool   put(const Slice & key,const Slice & value);
    Slice  get(const Slice & key);
    bool   remove(const Slice & key);
    bool   getError();
    void   write(const WriteBatch * pbatch);
    void   tWrite(WriteBatch * pbatch);
    void   compact();
    void   runBatchParallel(const WriteBatch * pbatch);

private:
    Options   	option;
    DBInternal *dbimpl;
    BaseCache * cache;
    Log       * log;
    int         errorStatus;
};

};
#endif