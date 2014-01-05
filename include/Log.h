#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

enum LOG_TYPE { LOG_DEBUG = 1, LOG_TRACE = 2, LOG_WARN  = 3, LOG_ERROR = 4, LOG_FATAL = 5};

extern std::ostream& operator<<(std::ostream& out, const LOG_TYPE value);

class Log
{
public:
    ~Log(void);
    static Log * GetInstance();

public:
    void 	SetLogInfo(const string & fileName,LOG_TYPE level);

    void 	_Trace(const char* format,...);
    void 	_Debug(const char* format,...);
    void 	_Warn(const char* format,...);
    void 	_Error(const char* format,...);
    void 	_Fatal(const char* format,...);

private:
    static Log * _pTheLogs;

private:
    Log();
    string GetLogFileName();
    void   WriteLog(LOG_TYPE outLevel,const char* format,va_list args);
    char * GetCurrentTm(int tag,char* buf,size_t size);

private:
    LOG_TYPE m_logLevel;
    string 	 m_prefix;
};

#endif