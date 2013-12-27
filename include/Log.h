#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
using namespace std;

#include <stdlib.h>

class Log
{
public:	
	~Log(void);

public:
	static void  SetLogMode(int level,String& module);
	static Log * GetInstance();

public:
	void 	_Trace(const char* format,...);
	void 	_Debug(const char* format,...);
	void 	_Warn(const char* format,...);
	void 	_Error(const char* format,...);
	void 	_Fatal(const char* format,...);

private:
	Log(void);
	String GetMutexName();
	String GetLogFile();
	char* GetCurrentTm(short tag,char* buf,size_t size);
	BOOL  GetLogDirectory(String& dir);
	void  WriteLog(int outLevel,const char* format,va_list args);
	void  SetModuleName(String& module);
	void  SetLogLevel(int level);

	FILE* OpenLogFile();
	void  CloseFile(FILE* pFile);
	void  Lock();
	void  Unlock();


public:
	enum{
		LOG_DEBUG = 1,
		LOG_TRACE = 2,
		LOG_WARN  = 3,
		LOG_ERROR = 4,
		LOG_FATAL = 5
	};

private:
	String	m_szModuleName;
	int		m_logLevel;
	short	m_runEnv;
	FILE*	m_pOutFile;
	HANDLE  m_hMutex;
	CRITICAL_SECTION m_csLock;
	static Log* _pTheLogs;
};

#endif