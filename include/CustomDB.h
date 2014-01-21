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
    CustomDB() : factory(NULL), cache(NULL) {  }

    virtual ~CustomDB()
    {
        if(factory) delete factory;
        if(cache)   delete cache;

        factory = NULL; cache = NULL;
    }

    void    close()
    {
        if(factory) delete factory;
        if(cache)   delete cache;

        factory = NULL; cache = NULL;
    }

    void    dump()
    {
        /**Todo List**/
        factory -> dump();
    }

    void   cleanCACHE()
    {
        cache -> clear();
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
};

#endif