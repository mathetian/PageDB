#include "CustomDB.h"
#include "FIFOLMCache.h"
#include "LRULMCache.h"

#include "PageDBImpl.h"

#include "EmptyCache.h"
#include "FileModule.h"
using namespace utils;

#include <string.h>

namespace customdb
{

CustomDB::CustomDB() : dbimpl(NULL), cache(NULL) {  }

CustomDB::~CustomDB()
{
    if(dbimpl) delete dbimpl;
    if(cache)   delete cache;

    dbimpl = NULL;
    cache = NULL;
}

void   CustomDB::close()
{
    if(dbimpl)  dbimpl -> fflush();

    if(dbimpl)  delete dbimpl;
    if(cache)   delete cache;

    dbimpl = NULL;
    cache = NULL;
}

void   CustomDB::dump()
{
    dbimpl -> dump();
}

void   CustomDB::cleanCACHE()
{
    cache -> clear();
}

void   CustomDB::destoryDB(const char * filename)
{
    string sfilename(filename, filename + strlen(filename));
    string idxName = sfilename + ".idx";
    string datName = sfilename +  ".dat";
    
    FileModule::Remove(idxName);
    FileModule::Remove(datName);

}

void CustomDB::fflush()
{
    dbimpl -> fflush();
}

void CustomDB::write(const WriteBatch * pbatch)
{
    dbimpl -> runBatch(pbatch);
}

void  CustomDB::tWrite(WriteBatch * pbatch)
{
    dbimpl -> write(pbatch);
}

void  CustomDB::compact()
{
    dbimpl -> compact();
}

void  CustomDB::runBatchParallel(const WriteBatch * pbatch)
{
    dbimpl -> runBatchParallel(pbatch);
}

bool  CustomDB::open(const Options & option)
{
    this -> option = option;

    log = Log::GetInstance();
    log -> SetLogInfo(option.logOption.logLevel, option.logOption.logPrefix, option.logOption.disabled);


    if(option.cacheOption.disabled == true)
    {
        cache = new EmptyCache();
    }
    else switch(option.cacheOption.cacheType)
        {
        case FIFO:
            cache = new FIFOLimitedMemoryCache();
            break;
        case LRU:
            cache = new LRULimitedMemoryCache();
            break;
        default:
            log -> _Fatal("CustomDB::open::cacheType error\n");
            break;
        }
    if(cache == NULL)
        log -> _Fatal("CustomDB::open::new cache error\n");

    switch(option.factoryOption.factoryType)
    {
    case EHASH:
        dbimpl = new PageDB;
        break;
    case CHASH:
        break;
    default:
        log -> _Fatal("CustomDB::open::dbimpl error\n");
    }

    if(dbimpl == NULL)
        log -> _Fatal("CustomDB::open::new dbimpl error\n");

    if(dbimpl -> init(option.fileOption.fileName) == false)
        log -> _Fatal("CustomDB::open::init dbimpl error\n");

    log -> _Trace("CustomDB::open initialization successfully\n");
    return true;
}

bool CustomDB::put(const Slice & key,const Slice & value)
{
    errorStatus = ERROR;

    if((cache -> get(key)).size() != 0)
        log -> _Warn("CustomDB::put::exist in cache\n");
    else
    {
        log -> _Trace("CustomDB::put::not exist in cache\n");

        if(!dbimpl -> put(key,value))
            log -> _Warn("CustomDB::put::dbimpl put error\n");
        else
        {
            log -> _Trace("CustomDB::put::dbimpl put successfully\n");
            cache -> put(key,value);
            errorStatus = SUCCE;
        }
    }
    return errorStatus;
}

Slice CustomDB::get(const Slice & key)
{
    errorStatus = ERROR;
    Slice rs = cache -> get(key);
    if(rs.size() != 0)
        errorStatus = SUCCE;
    else
    {
        log -> _Trace("CustomDB::get::dbimpl not in cache\n");
        rs = dbimpl -> get(key);
        if(rs.size() == 0)
            log -> _Warn("CustomDB::get::dbimpl get warning\n");
        else
        {
            cache -> put(key,rs);
            errorStatus = SUCCE;
        }
    }

    return rs;
}

bool CustomDB::remove(const Slice & key)
{
    errorStatus = ERROR;

    Slice rs = cache -> get(key);
    if(rs.size() != 0)
    {
        if((cache -> remove(key)) == 0)
        {
            log -> _Error("CustomDB::remove::cache remove error\n");
            return errorStatus;
        }
        else log -> _Trace("CustomDB::remove::cache remove successfully\n");
    }

    if((dbimpl -> remove(key)) == 0)
        log -> _Error("CustomDB::remove::dbimpl remove error\n");

    errorStatus = SUCCE;
    return errorStatus;
}

bool CustomDB::getError()
{
    return errorStatus;
}

};