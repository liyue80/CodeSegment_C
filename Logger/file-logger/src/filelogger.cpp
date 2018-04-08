#include "filelogger.h"
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <Pathcch.h>
#include <assert.h>
#include <Winerror.h>

#pragma comment(lib, "Pathcch.lib")

//
// Configuration for the logger file.
static LPCTSTR COMPANY_NAME = TEXT("Company");
static LPCTSTR MODULE_NAME = TEXT("Module");
static LPCTSTR LOGGER_FNAME = TEXT("MyLogger.txt");

//
// Global variables.
static TRACE_LEVEL gLogLevel = TRACE_LEVEL_NONE;
static unsigned gMaxFileSize = 32;
const static unsigned MAX_TRACE_LOG_STRING_LENGTH = 1024;


class LogMutexObject {
public:
	LogMutexObject() {
		InitializeCriticalSection(&this->m_mutex);
	}
	~LogMutexObject() {
		DeleteCriticalSection(&this->m_mutex);
	}
	void Lock() {
		EnterCriticalSection(&this->m_mutex);
	}
	void Unlock() {
		LeaveCriticalSection(&this->m_mutex);
	}
private:
	CRITICAL_SECTION m_mutex;
};

LogMutexObject gMutex;

// Close the file, move it and create/open a new one.
// Return the new file handle.
static HANDLE _promLogMoveFile(HANDLE hFileToClose, LPCTSTR pPath, LPCTSTR pFrom)
{
	TCHAR szFrom[MAX_PATH] = {0};
	TCHAR szTo[MAX_PATH] = {0};

	assert(hFileToClose != INVALID_HANDLE_VALUE);
	assert(pPath != NULL);
	assert(pFrom != NULL);

	CloseHandle(hFileToClose);

	if (PathCchCombine(szFrom, MAX_PATH, pPath, pFrom) != S_OK)
		return INVALID_HANDLE_VALUE;

	_stprintf_s(szTo, MAX_PATH, TEXT("%s_bak"), szFrom);

	if (!MoveFileEx(szFrom, szTo, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
	{
		return INVALID_HANDLE_VALUE;
	}

	HANDLE hNewFile = CreateFile(szFrom, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwFileSize = SetFilePointer(hNewFile, 0, NULL, FILE_END);
	if (INVALID_SET_FILE_POINTER == dwFileSize)
	{
		CloseHandle(hNewFile);
		return INVALID_HANDLE_VALUE;
	}
	return hNewFile;
}

static BOOL _promLogCreate(OUT HANDLE *phFile)
{
    size_t len = 0;
    TCHAR szProgramDataPath[MAX_PATH] = {0};

    if (!phFile)
        return FALSE;

	if (_tgetenv_s(&len, szProgramDataPath, MAX_PATH, TEXT("ProgramData")) != 0)
        return FALSE;

	TCHAR szCompanyPath[MAX_PATH] = {0};
	if (FAILED(PathCchCombine(szCompanyPath, MAX_PATH, szProgramDataPath, COMPANY_NAME)))
		return FALSE;

    if (!CreateDirectory(szCompanyPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            return FALSE;
    }

	TCHAR szModulePath[MAX_PATH] = {0};
	if (FAILED(PathCchCombine(szModulePath, MAX_PATH, szCompanyPath, MODULE_NAME)))
		return FALSE;

    if (!CreateDirectory(szModulePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            return FALSE;
    }

	TCHAR szFile[MAX_PATH] = {0};
	PathCchCombine(szFile, MAX_PATH, szModulePath, LOGGER_FNAME);

	HANDLE hFile = CreateFile(szFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		DWORD dwError = GetLastError();
		printf("CreateFile error (%u)\n", (unsigned)dwError);
		return FALSE;
	}

	LONG lFileSizeHi = 0;
	DWORD dwFileSize = SetFilePointer(hFile, 0, &lFileSizeHi, FILE_END);
	if (INVALID_SET_FILE_POINTER == dwFileSize)
	{
		return FALSE;
	}

	// Do retention
	if (gMaxFileSize != 0)
	{
		dwFileSize >>= 20; // Convert to Mega-bytes
		if (lFileSizeHi != 0 || dwFileSize >= gMaxFileSize)
		{
			if ((hFile = _promLogMoveFile(hFile, szModulePath, LOGGER_FNAME)) == INVALID_HANDLE_VALUE)
			{
				return FALSE;
			}
		}
	}

	*phFile = hFile;
    return TRUE;
}

static void _promLogMsg(LPCTSTR msg)
{
	time_t timer = 0;
	TCHAR buffer[MAX_TRACE_LOG_STRING_LENGTH] = {0};
	struct tm tm_info = {0};
	HANDLE hFile = INVALID_HANDLE_VALUE;

	__try {
		gMutex.Lock();
		if (!_promLogCreate(&hFile))
		{
			_ftprintf(stderr, TEXT("Logfile has not been initialized\n"));
			return;
		}

		// Format current time
		time(&timer);
		if (localtime_s(&tm_info, &timer) != 0)
		{
			_ftprintf(stderr, TEXT("localtime_s failed.\n"));
			return;
		}
		_tcsftime(buffer, MAX_TRACE_LOG_STRING_LENGTH, TEXT("[%Y-%m-%d %H:%M:%S]"), &tm_info);
		_tcscat_s(buffer, MAX_TRACE_LOG_STRING_LENGTH, msg);
		_tcscat_s(buffer, MAX_TRACE_LOG_STRING_LENGTH, TEXT("\r\n"));
		WriteFile(hFile, (LPCVOID)buffer, (DWORD)(_tcslen(buffer)*sizeof(TCHAR)), NULL, NULL);
		FlushFileBuffers(hFile);
		CloseHandle(hFile);
	}
	__finally {
		gMutex.Unlock();
	}
	return;
}

static void _promLogMsg(LPCTSTR fmt, va_list ap)
{
	TCHAR buf[MAX_TRACE_LOG_STRING_LENGTH] = {0};
	_vsntprintf_s(buf, MAX_TRACE_LOG_STRING_LENGTH, fmt, ap);
	_promLogMsg(buf);
}

static UINT32 getByteHexString(const UINT8 * pu8Data, UINT32 u32DataLen, UINT32 u32StartIndex, 
						LPTSTR pstrHex, UINT32 u32StrSize)
{
    UINT32 u32Ret = 0;
    UINT32 u32Limited = 0;
    TCHAR strTemp[8];
	CONST static UINT32 BYTE_HEXSTR_HEADER_LENGTH = 13;
    CONST static UINT32 MIN_BYTE_HEXSTR_LENGTH = BYTE_HEXSTR_HEADER_LENGTH + 2;
	CONST static UINT32 MAX_BYTE_PER_HEXSTR_LINE = 16;
	
    if ((pu8Data == NULL) || (u32DataLen == 0) || 
        (u32StartIndex >= u32DataLen) || (pstrHex == NULL) || 
        (u32StrSize < MIN_BYTE_HEXSTR_LENGTH))
    {
        u32Ret = 0;    
    }
    else
    {
        u32Ret = u32DataLen - u32StartIndex;
        u32Limited = (u32StrSize - BYTE_HEXSTR_HEADER_LENGTH) / 3;
        if (u32Limited > MAX_BYTE_PER_HEXSTR_LINE)
        {
            u32Limited = MAX_BYTE_PER_HEXSTR_LINE;
        }
        
        if (u32Limited > u32Ret)
        {
            u32Limited = u32Ret;
        }
        
		_stprintf_s(pstrHex, u32StrSize, TEXT("%04x-%04x:"), u32StartIndex, (u32StartIndex + u32Limited - 1));
        u32Ret = 0;
        while (u32Ret < u32Limited)
        {
			_stprintf_s(strTemp, _countof(strTemp), TEXT(" %02x"), pu8Data[u32StartIndex+u32Ret]);
            _tcscat_s(pstrHex, u32StrSize, strTemp);
            u32Ret++;
        }
    }
    
    return u32Ret;
}

int PromLogSetLevel(TRACE_LEVEL level)
{
	if (level < 0 || level > 3)
		return -1;
	gLogLevel = level;
	return 0;
}

TRACE_LEVEL PromLogGetLevel(void)
{
	return gLogLevel;
}

void PromLogInfo(LPCTSTR fmt, ...)
{
	if (gLogLevel < TRACE_LEVEL_INFO)
		return;

	va_list vp = NULL;
	va_start(vp, fmt);
	_promLogMsg(fmt, vp);
	va_end(vp);
}

void PromLogError(LPCTSTR fmt, ...)
{
	if (gLogLevel < TRACE_LEVEL_ERR)
		return;

	va_list vp = NULL;
	va_start(vp, fmt);
	_promLogMsg(fmt, vp);
	va_end(vp);
}

void PromLogMsg(LPCTSTR fmt, ...)
{
	if (gLogLevel == TRACE_LEVEL_NONE)
		return;

	va_list vp = NULL;
	va_start(vp, fmt);
	_promLogMsg(fmt, vp);
	va_end(vp);
}

void PromLogData(LPCBYTE pdata, unsigned long length, LPCTSTR fmt, ...)
{
	if (gLogLevel < 3)
		return;

	if (NULL == pdata || 0 == length)
		return;

	TCHAR buf[MAX_TRACE_LOG_STRING_LENGTH] = {0};
	TCHAR strOfLength[64] = {0};
	va_list vp = NULL;
	va_start(vp, fmt);
	_vstprintf_s(buf, _countof(buf), fmt, vp);
	va_end(vp);
	_stprintf_s(strOfLength, _countof(strOfLength), TEXT(" (DATA length %d)"), length);
	_tcscat_s(buf, MAX_TRACE_LOG_STRING_LENGTH, strOfLength);
	_promLogMsg(buf);

	unsigned u32Index = 0;
	unsigned u32Logged = 1;
	while((u32Index < length) && (u32Logged != 0))
	{
		u32Logged = getByteHexString(pdata, length, u32Index, buf, sizeof(buf));
		if (u32Logged != 0)
		{
			_promLogMsg(buf);
			u32Index += u32Logged;
		}
	}

    return;
}

unsigned PromLogSetMaxFileSize(unsigned size)
{
	unsigned ret = gMaxFileSize;
	gMaxFileSize = size;
	return ret;
}