#ifndef _OPTION_H
#define _OPTION_H

#define CUSTOMDB_W  0
#define CUSTOMDB_R  2
#define CUSTOMDB_C  4

#include <string>
using namespace std;

#include "Log.h"

typedef struct
{
    string fileName;
    unsigned int read_write : 2;
    unsigned int creat     : 1;
} FileOption;

typedef struct
{
    FileOption foption;
} EnvOption;

#define FIFO 0
#define  LRU 1

typedef struct _tCacheOption{
    int cacheType;
    int sizeLimit;
    _tCacheOption()
    {
        sizeLimit = 4; 
        cacheType = FIFO;
    }
} CacheOption;

#define EHASH 0
#define CHASH 1

typedef struct _tFactoryOption{
    int factoryType;
    _tFactoryOption()
    { factoryType = EHASH; }
} FactoryOption;

typedef struct _tOptions
{
    LOG_TYPE 	  logLevel;
    string 	      logPrefix, dbFilePrefix;
    EnvOption     envOption;
    CacheOption   cacheOption;
    FactoryOption factoryOption;

    _tOptions()
    {
        logLevel = LOG_WARN;
        logPrefix = "demo";
        dbFilePrefix = "demo";
    } 
}Options;


#endif