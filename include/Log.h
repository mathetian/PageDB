#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
using std::string;
using std::ostream;

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../utils/Thread.h"
using utils::Mutex;

namespace customdb
{

enum LOG_TYPE { LOG_DEBUG = 1, LOG_TRACE = 2, LOG_WARN  = 3, LOG_ERROR = 4, LOG_FATAL = 5};

extern ostream& operator<<(ostream& out, const LOG_TYPE value);

class Log
{
public:
    static Log * GetInstance();
    void    SetLogInfo(LOG_TYPE level, const char * fileName, bool disabled = false);

public:
    ~Log()  { }
    void 	_Trace(const char* format,...);
    void 	_Debug(const char* format,...);
    void 	_Warn(const char* format,...);
    void 	_Error(const char* format,...);
    void 	_Fatal(const char* format,...);

private:
    static Log *     m_pTheLogs;
    static LOG_TYPE  m_logLevel;
    static string    m_prefix;
    static FILE *    m_pfile;
    static Mutex     m_mutex;
    static bool      m_disabled;

private:
    Log() {}
    string GetLogFileName();
    void   WriteLog(LOG_TYPE outLevel,const char* format,va_list args);
    void   GetCurrentTm(int tag, size_t size, char * buf);
};

};
#endif