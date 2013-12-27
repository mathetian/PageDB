#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

class Log
{
public:	
	~Log(void);
	static Log * GetInstance();

public:
	void 	SetLogInfo(const string & fileName,int level);
	
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
	void   WriteLog(int outLevel,const char* format,va_list args);
	char * GetCurrentTm(int tag,char* buf,size_t size);
	bool   GetLogDirectory(string& dir);

public:
	enum{ LOG_DEBUG = 1, LOG_TRACE = 2, LOG_WARN  = 3, LOG_ERROR = 4, LOG_FATAL = 5};
private:
	int		m_logLevel;
	FILE *	m_pOutFile;
	string 	m_prefix;
};

#endif