#ifndef _OPTION_H
#define _OPTION_H

#include "Log.h"
using namespace utils;

namespace pagedb
{

/******************************/
#define CUSTOMDB_W  0
#define CUSTOMDB_R  1
#define CUSTOMDB_C  3

struct FileOption_t {
    unsigned int read_write : 2;
    unsigned int creat      : 1;
    const char * fileName;
    FileOption_t() : read_write(3), creat(1), \
        fileName("demo") { }
};

/******************************/
#define  LRU  0
#define FIFO  1
#define EMPTY 2
#define SLOTNUM 1000

struct CacheOption_t {
    int  cacheType;
    bool disabled;
    int  slotNum;

    CacheOption_t() {
        slotNum        = SLOTNUM;
        cacheType      = FIFO;
        disabled       = false;
    }
};

/******************************/
#define EHASH 0
#define CHASH 1

struct FactoryOption_t {
    int factoryType;
    FactoryOption_t() {
        factoryType = EHASH;
    }
};

/******************************/
struct LogOption_t {
    Log::LOG_TYPE     logLevel;
    const char * logPrefix;
    bool         disabled;
    LogOption_t() {
        logLevel  = Log::LOG_WARN;
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

struct Options_t {
    CacheOption   cacheOption;
    FactoryOption factoryOption;
    LogOption     logOption;
    FileOption    fileOption;
};

typedef struct Options_t Options;

};


#endif