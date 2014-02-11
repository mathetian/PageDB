#include "CustomDB.h"

bool CustomDB::open(const Options & option)
{
    this -> option = option;
    
    log = Log::GetInstance();
    log -> SetLogInfo(option.logOption.logLevel, option.logOption.logPrefix, option.logOption.disabled);
    
    switch(option.cacheOption.cacheType)
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
        factory = new ExtendibleHash;
        break;
    case CHASH:
        factory = new ChainHash;
        break;
    default:
        log -> _Fatal("CustomDB::open::factory error\n");
    }
    if(factory == NULL)
        log -> _Fatal("CustomDB::open::new factory error\n");
    
    if(factory -> init(option.fileOption.fileName) == false)
        log -> _Fatal("CustomDB::open::init factory error\n");
    
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
        
        if(!factory -> put(key,value))
            log -> _Warn("CustomDB::put::factory put error\n");
        else
        {
            log -> _Trace("CustomDB::put::factory put successfully\n");
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
        log -> _Trace("CustomDB::get::factory not in cache\n");
        rs = factory -> get(key);
        if(rs.size() == 0)
            log -> _Warn("CustomDB::get::factory get warning\n");
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

    if((factory -> remove(key)) == 0)
        log -> _Error("CustomDB::remove::factory remove error\n");
    
    errorStatus = SUCCE;
    return errorStatus;
}

bool CustomDB::getError()
{
    return errorStatus;
}