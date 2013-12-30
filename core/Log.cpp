#include "../include/Log.h"

#include  <time.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#define TIME_SHORT 50
#define TIME_FULL  51

#define TM_FORMAT_SHORT     "%04d%02d%02d"
#define TM_FORMAT_FULL      "%04d.%02d.%02d - %02d:%02d:%02d"

Log* Log::_pTheLogs = NULL;

Log::Log(void)
{
	m_logLevel = LOG_WARN;
}

Log::~Log(void)
{
}

void Log::SetLogInfo(const string & prefix,int level)
{
	if(_pTheLogs == NULL){
		_pTheLogs = new Log;
	}
	_pTheLogs -> m_prefix   = prefix;
	_pTheLogs -> m_logLevel = level;
}

Log* Log::GetInstance()
{
	if(_pTheLogs == NULL){
		_pTheLogs = new Log;
	}
	return _pTheLogs;
}

string Log::GetLogFileName()
{
	string log = m_prefix;
	log.append(".log");
	return log;
}

bool  Log::GetLogDirectory(string& dir)
{
	/*if( !Log::GetWorkspace(dir)){
		return FALSE;
	}
	
	char dateBuf[32] = {0x00};
	GetCurrentTm(TIME_SHORT,dateBuf,32);

	dir.append("\\log\\");
	dir.append(dateBuf);

	if( !Log::FolderExist(dir) ){
		Log::CreateFolder(dir);
	}
*/
	return true;
}

char* Log::GetCurrentTm(int tag,char* buf,size_t size)
{
	time_t now;
	struct tm *ptime = NULL;
	time(&now);
	ptime = localtime(&now);

	if(ptime != NULL){
		switch(tag)
		{
		case TIME_FULL :
			snprintf(buf,size,TM_FORMAT_FULL,(ptime->tm_year + 1900),(ptime->tm_mon + 1),ptime->tm_mday 
				,ptime->tm_hour,ptime->tm_min,ptime->tm_sec);
			break;
		case TIME_SHORT :
		default:
			snprintf(buf,size,TM_FORMAT_SHORT,(ptime->tm_year + 1900),(ptime->tm_mon + 1),ptime->tm_mday);
			break;
		}
		return buf;
	}

	return NULL;
}

void Log::WriteLog(int outLevel,const char* format,va_list args)
{
	if(outLevel > m_logLevel){
		printf("WriteLog::outLevel::format::error\n");
		exit(1);
	}
	
	FILE* pfile = fopen(GetLogFileName().c_str(), "a+");
	if(pfile == NULL){
		return ;
	}

	char buf[32]={0x00};
	GetCurrentTm(TIME_FULL, buf, 32);
	
	fprintf (pfile,"%s\t",buf);
	vfprintf(pfile,format,args);
	fprintf (pfile, "\n");
	fflush(pfile);
	fclose(pfile);
	pfile = NULL;
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