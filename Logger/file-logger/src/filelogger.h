#pragma once
#include <Windows.h>
#include <tchar.h>

#ifndef _FILE_LOGGER_325D5AC6_
#define _FILE_LOGGER_325D5AC6_

enum TRACE_LEVEL {
	TRACE_LEVEL_NONE = 0,
	TRACE_LEVEL_ERR,
	TRACE_LEVEL_INFO,
	TRACE_LEVEL_DATA
};

TRACE_LEVEL PromLogGetLevel(void);
int PromLogSetLevel(TRACE_LEVEL level);
unsigned PromLogSetMaxFileSize(unsigned size);
void PromLogInfo(LPCTSTR fmt, ...);
void PromLogError(LPCTSTR fmt, ...);
void PromLogMsg(LPCTSTR fmt, ...);
void PromLogData(LPCBYTE pdata, unsigned long length, LPCTSTR fmt, ...);

#endif