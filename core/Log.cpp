#include "Log.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#define TIME_SHORT 50
#define TIME_FULL  51

#define TM_FORMAT_SHORT     "%04d%02d%02d"
#define TM_FORMAT_FULL      "%04d.%02d.%02d - %02d:%02d:%02d"

Log   *  Log::_pTheLogs = NULL;
LOG_TYPE Log::m_logLevel;
string   Log::m_prefix;
FILE  *  Log::pfile;

const char * getStrFromType(const LOG_TYPE & value)
{
#define MAPENTRY(p) {p, #p}
    const struct MapEntry{
        LOG_TYPE value;
        const char* str;
    } entries[] = {
        MAPENTRY(LOG_DEBUG),
        MAPENTRY(LOG_TRACE),
        MAPENTRY(LOG_WARN),
        MAPENTRY(LOG_ERROR),
        MAPENTRY(LOG_FATAL),
        {LOG_DEBUG, 0}//doesn't matter what is used instead of ErrorA here...
    };
#undef MAPENTRY
    const char* s = 0;
    for (const MapEntry* i = entries; i->str; i++){
        if (i->value == value){
            s = i->str;
            break;
        }
    }
    return s;
}

void Log::SetLogInfo(LOG_TYPE level, const char * prefix)
{
    if(_pTheLogs == NULL) 
        _pTheLogs = new Log;

    _pTheLogs -> m_prefix   = prefix;
    _pTheLogs -> m_logLevel = level;
    _pTheLogs -> pfile = fopen(GetLogFileName().c_str(), "a+");
    if(! _pTheLogs -> pfile)
        printf("open file error\n");
}

Log* Log::GetInstance()
{
    if(_pTheLogs == NULL)
        _pTheLogs = new Log;
    
    return _pTheLogs;
}

string Log::GetLogFileName()
{
    string log = m_prefix;
    log.append(".log");
    return log;
}

void Log::GetCurrentTm(int tag, size_t size, char * buf)
{
    time_t now; time(&now);
    
    struct tm *ptime = NULL;
    ptime = localtime(&now);

    if(ptime != NULL)
    {
        switch(tag)
        {
        case TIME_FULL :
            sprintf(buf,TM_FORMAT_FULL,(ptime->tm_year + 1900),(ptime->tm_mon + 1),ptime->tm_mday
                     ,ptime->tm_hour,ptime->tm_min,ptime->tm_sec);
            break;
        case TIME_SHORT :
        default:
            sprintf(buf,TM_FORMAT_SHORT,(ptime->tm_year + 1900),(ptime->tm_mon + 1),ptime->tm_mday);
            break;
        }
    }
}

void Log::WriteLog(LOG_TYPE outLevel,const char* format,va_list args)
{    
    /**
        Need advanced solution
    **/

    const char * str = getStrFromType(outLevel);
    char buf[256];memset(buf, 0, sizeof(buf));
    
    GetCurrentTm(TIME_FULL, 32, buf);

    char fformat[1000];
    sprintf(fformat,"%s\t%s--%s",buf,str,format);

    va_list old; va_copy(old,args);
    
    vfprintf(pfile,fformat,args);
    fprintf(pfile,"\n");

    if(m_logLevel <= outLevel)
        vprintf(fformat,old);
    
    fflush(pfile);
    
    if(m_logLevel < outLevel)
        exit(1);
}

void Log::_Trace(const char* format,...)
{
    va_list args;
    va_start(args,format);
    WriteLog(LOG_TRACE,format,args);
    va_end(args);
}

void Log::_Debug(const char* format,...)
{
    va_list args;
    va_start(args,format);
    WriteLog(LOG_DEBUG,format,args);
    va_end(args);
}

void Log::_Warn(const char* format,...)
{
    va_list args;
    va_start(args,format);
    WriteLog(LOG_WARN,format,args);
    va_end(args);
}

void Log::_Error(const char* format,...)
{
    va_list args;
    va_start(args,format);
    WriteLog(LOG_ERROR,format,args);
    va_end(args);
}

void Log::_Fatal(const char* format,...)
{
    va_list args;
    va_start(args,format);
    WriteLog(LOG_FATAL,format,args);
    va_end(args);
}