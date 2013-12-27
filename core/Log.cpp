#include "../include/Log.h"

#include  <time.h> 
#include <stdio.h>
#include <stdlib.h>


#define TIME_SHORT 50
#define TIME_FULL  51

#define TM_FORMAT_SHORT     "%04d%02d%02d"
#define TM_FORMAT_FULL      "%04d.%02d.%02d - %02d:%02d:%02d"

Log* Log::_pTheLogs = NULL;

Log::Log(void)
{
	m_logLevel = LOG_DEBUG;
}

Log::~Log(void)
{
}

void Log::SetLogMode(int level,String& logFile)
{
	if(_pTheLogs == NULL){
		_pTheLogs = new Log;
	}

	_pTheLogs -> SetLogLevel(level);
	_pTheLogs -> SetModuleName(logFile);
}

Log* Log::GetInstance()
{
	if(_pTheLogs == NULL){
		_pTheLogs = new Log;
	}

	return _pTheLogs;
}

String Log::GetLogFile()
{
	String log = m_szModuleName;
	log.append(".log");
	return log;
}

void Log::SetModuleName(String& module)
{
	m_szModuleName = module;	
}

void Log::SetLogLevel(int level)
{
	m_logLevel = level;
}

BOOL  Log::GetLogDirectory(String& dir)
{
	if( !Log::GetWorkspace(dir)){
		return FALSE;
	}
	
	char dateBuf[32] = {0x00};
	GetCurrentTm(TIME_SHORT,dateBuf,32);

	dir.append("\\log\\");
	dir.append(dateBuf);

	if( !Log::FolderExist(dir) ){
		Log::CreateFolder(dir);
	}

	return TRUE;
}

FILE* Log::OpenLogFile()
{
	String logDir;
	if(!GetLogDirectory(logDir)) return NULL;

	String szlogName;
	szlogName = GetLogFile();

	logDir.append("\\");
	logDir.append(szlogName);

	return fopen(logDir.c_str(), "a+");    
}

void Log::CloseFile(FILE* pFile)
{
	fflush(pFile);
	::fclose(pFile);
	 pFile = NULL;
}

char* Log::GetCurrentTm(short tag,char* buf,size_t size)
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
	if(outLevel < m_logLevel){
		return ;
	}
	
	FILE* pfile = OpenLogFile();
	if(pfile == NULL){
		return ;
	}

	char buf[32]={0x00};
	GetCurrentTm(TIME_FULL,buf,32);
	
	fprintf (pfile,"%s\t",buf);
	vfprintf(pfile,format,args);
	fprintf (pfile, "\n");
	fflush(pfile);

	CloseFile(pfile);
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