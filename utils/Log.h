// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _LOG_H
#define _LOG_H

#include "Noncopyable.h"
#include "Multithreading.h"
/**
** Log module is used to store log information into files and is thread-safe.
**/
namespace utils
{

class Log : Noncopyable
{
public:
    /**
    ** The level of Log is divided into five parts. DEBUG/TRACE/WARN/ERROR/FATAL
    ** In generl, we should set LOG_TRACE as the default level of Logging.
    **/

    enum LOG_TYPE { LOG_DEBUG, LOG_TRACE, LOG_WARN, LOG_ERROR, LOG_FATAL};

    /**
    ** Log belongs to Singleton.
    **/
    static Log * GetInstance();
    /**
    Initizate the log level and stream
    **/
    void    SetLogInfo(LOG_TYPE level, const char * fileName, bool disabled = false);

public:
    void 	_Trace(const char* format,...);
    void 	_Debug(const char* format,...);
    void 	_Warn(const char* format,...);
    void 	_Error(const char* format,...);
    void 	_Fatal(const char* format,...);

private:
    /**
    ** Disable the log construction
    **/
    Log() {}
    /**
    ** We set the log filename in `SetLogInfo`
    ** We use `GetLogFileName` to append the posfix to filename
    **/
    string GetLogFileName();
    /**
    ** _XXXX will use the WriteLog as the internal function
    ** WriteLog only write log item which the level of it is bigger than the setted level.
    ** Also, it will check whether we have set disable attribute.
    ** When the level is bigger than setted level, our progame will exit.
    **/
    void   WriteLog(LOG_TYPE outLevel,const char* format,va_list args);
    /**
    ** Get Timestamp. There are two kinds of timestamp.
    **/
    void   GetCurrentTm(int tag, size_t size, char * buf);
private:
    static Log *     m_pTheLogs;
    static LOG_TYPE  m_logLevel;
    static string    m_prefix;
    static FILE *    m_pfile;
    static Mutex     m_mutex;
    static bool      m_disabled;
};

/**
** Print the LOG_TYPE as readable string
**/
extern ostream& operator<<(ostream& out, const Log::LOG_TYPE value);

};
#endif