#ifndef _OPTION_H
#define _OPTION_H

#include <string>
using namespace std;

#include "Log.h"

namespace customdb
{

/******************************/
#define CUSTOMDB_W  0
#define CUSTOMDB_R  1
#define CUSTOMDB_C  3

struct FileOption_t
{
    unsigned int read_write : 2;
    unsigned int creat      : 1;
    const char * fileName;
    FileOption_t() : read_write(3), creat(1), \
        fileName("demo") { }
};

/******************************/
#define FIFO 0
#define  LRU 1

struct CacheOption_t
{
    int cacheType;
    int cacheLimitInMB;
    CacheOption_t()
    {
        cacheLimitInMB = 4;
        cacheType = FIFO;
    }
};

/******************************/
#define EHASH 0
#define CHASH 1

struct FactoryOption_t
{
    int factoryType;
    FactoryOption_t()
    {
        factoryType = EHASH;
    }
};

/******************************/
struct LogOption_t
{
    LOG_TYPE     logLevel;
    const char * logPrefix;
    bool         disabled;
    LogOption_t()
    {
        logLevel  = LOG_WARN;
        logPrefix = "demo";
        disabled  = false;
    }
};

/******************************/

typedef struct FileOption_t    FileOption;
typedef struct CacheOption_t   CacheOption;
typedef struct FactoryOption_t FactoryOption;
typedef struct LogOption_t     LogOption;

/*******************************/

struct Options_t
{
    CacheOption   cacheOption;
    FactoryOption factoryOption;
    LogOption     logOption;
    FileOption    fileOption;
};

typedef struct Options_t Options;

};


#endif