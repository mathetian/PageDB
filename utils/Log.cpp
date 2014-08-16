// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Log.h"

namespace utils
{

#define TIME_SHORT 50
#define TIME_FULL  51

#define TM_FORMAT_SHORT     "%04d%02d%02d"
#define TM_FORMAT_FULL      "%04d.%02d.%02d - %02d:%02d:%02d"

Log   *  Log::m_pTheLogs = NULL;
Log::LOG_TYPE Log::m_logLevel;
string   Log::m_prefix;
FILE  *  Log::m_pfile;
Mutex    Log::m_mutex;
bool     Log::m_disabled;

/**
** Print type as readable string
**/
const char * getStrFromType(const Log::LOG_TYPE & value)
{
#define MAPENTRY(p) {p, #p}
    const struct MapEntry
    {
        Log::LOG_TYPE value;
        const char* str;
    } entries[] =
    {
        MAPENTRY(Log::LOG_DEBUG),
        MAPENTRY(Log::LOG_TRACE),
        MAPENTRY(Log::LOG_WARN),
        MAPENTRY(Log::LOG_ERROR),
        MAPENTRY(Log::LOG_FATAL),
        {Log::LOG_DEBUG, 0}//doesn't matter what is used instead of ErrorA here...
    };
#undef MAPENTRY
    const char* s = 0;
    for (const MapEntry* i = entries; i->str; i++)
    {
        if (i->value == value)
        {
            s = i->str;
            break;
        }
    }
    return s;
}

void Log::SetLogInfo(LOG_TYPE level, const char * prefix, bool disabled)
{
    if(m_pTheLogs == NULL)
        m_pTheLogs = new Log;

    m_pTheLogs -> m_prefix   = prefix;
    m_pTheLogs -> m_logLevel = level;
    m_pTheLogs -> m_pfile = fopen(GetLogFileName().c_str(), "a+");
    if(! m_pTheLogs -> m_pfile)
        printf("open file error\n");

    m_disabled = disabled;
}

Log* Log::GetInstance()
{
    if(m_pTheLogs == NULL)
        m_pTheLogs = new Log;

    return m_pTheLogs;
}

string Log::GetLogFileName()
{
    string log = m_prefix;
    log.append(".log");
    return log;
}

void Log::GetCurrentTm(int tag, size_t size, char * buf)
{
    time_t now;
    time(&now);

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

/**
** WriteLog only write log item which the level of it is bigger than the setted level.
** Also, it will check whether we have set disable attribute.
** When the level is bigger than setted level, our progame will exit.
**/
void Log::WriteLog(LOG_TYPE outLevel,const char* format,va_list args)
{

    if(m_disabled == true)
    {
        if(m_logLevel > outLevel)
            return;
    }

    const char * str = getStrFromType(outLevel);
    char buf[256];
    memset(buf, 0, sizeof(buf));

    GetCurrentTm(TIME_FULL, 32, buf);

    char fformat[1000];
    sprintf(fformat,"%s\t%s--%s",buf,str,format);

    va_list old;
    va_copy(old,args);

    if(m_disabled == true)
    {
        m_mutex.lock();
        vfprintf(m_pfile,fformat,args);
        fprintf(m_pfile,"\n");
        m_mutex.unlock();
    }

    if(m_logLevel <= outLevel)
        vprintf(fformat,old);

    fflush(m_pfile);

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

};