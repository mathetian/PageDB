#include "CustomDB.h"
#include "FFLMC.h"
#include "EHash.h"
#include "CHash.h"

CustomDB::CustomDB()
{
}

CustomDB::~CustomDB()
{
    if(factory)
    {
        delete factory;
        factory = NULL;
    }

    if(cache)
    {
        delete cache;
        cache=NULL;
    }
}

bool CustomDB::open(const Options&option)
{
    this -> option = option;

    log = Log::GetInstance();
    
    log -> SetLogInfo(option.logPrefix, option.logLevel);

    switch(option.cacheOption.cacheType)
    {
    case FIFO:
        cache = new FIFOLimitedMemoryCache();
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
        factory = new ExtendibleHash();
        break;
    case CHASH:
        factory = new ChainHash();
        break;
    default:
        log -> _Fatal("CustomDB::open::factory error\n");
    }

    if(factory == NULL)
        log -> _Fatal("CustomDB::open::new factory error\n");

    if(factory -> init(option.dbFilePrefix) == false)
        log -> _Fatal("CustomDB::open::init factory error\n");

    log -> _Trace("CustomDB::open initialization successfully\n");
}

bool CustomDB::put(const string&key,const string&value)
{
    errorStatus = ERROR;
    if((cache -> get(key)).size() != 0)
        log -> _Warn("CustomDB::put::exist in cache, %s %s\n", key.c_str(), value.c_str());
    else
    {
        log -> _Trace("CustomDB::put::not exist in cache\n");
        
        if(!factory -> put(key,value))
            log -> _Warn("CustomDB::put::factory put error, %s %s\n",key.c_str(), value.c_str());
        else
        {
            log -> _Trace("CustomDB::put::factory put successfully\n");
            cache -> put(key,value);
            errorStatus = SUCCE;
        }
    }
    return errorStatus;
}

string CustomDB::get(const string&key)
{
    errorStatus = ERROR;
    string rs = cache -> get(key);
    if(rs.size() != 0) errorStatus = SUCCE;
    else
    {
        rs = factory -> get(key);
        if(rs.size() == 0)
            log -> _Warn("CustomDB::get::factory get error, %s\n",key.c_str());
        else
        {
            cache -> put(key,rs);
            errorStatus = SUCCE;
        }
    }
    return rs;
}

bool CustomDB::remove(const string&key)
{
    errorStatus = ERROR;
    string rs = cache -> get(key);
    if(rs.size() != 0)
    {
        if((cache -> remove(key)) == 0)
        {
            log -> _Error("CustomDB::remove::cache remove error\n");
            return errorStatus;
        }
    }
    if((factory -> remove(key)) == 0)
        log -> _Error("CustomDB::remove::factory remove error\n");
    errorStatus = SUCCE;
    return errorStatus;
}

bool CustomDB::getError()
{
    return errorStatus;
}