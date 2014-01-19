#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "Log.h"
#include "Slice.h"
#include "Option.h"
#include "Factory.h"
#include "CacheImpl.h"
#include "FactoryImpl.h"

#include <string>
using namespace std;

#define ERROR 0
#define SUCCE 1

class CustomDB
{
public:
    CustomDB() { }

    virtual ~CustomDB()
    {
        if(factory) delete factory;
        if(cache)   delete cache;
        if(idxFile) fclose(idxFile);
        if(datFile) fclose(datFile);

        factory = NULL; cache = NULL;
    }

public:
    bool 	open(const Options & option);
    bool 	put(const Slice & key,const Slice & value);
    Slice   get(const Slice & key);
    bool 	remove(const Slice & key);
    
    bool	getError();

private:
    Options   	option;
    Factory   * factory;
    BaseCache * cache;
    Log       * log;
    int         errorStatus;

public:
    bool init();

private:
    FILE	 * idxFile;
    FILE     * datFile;
};

#endif