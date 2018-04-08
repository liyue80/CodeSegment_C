#include <Windows.h>
#include "../src/filelogger.h"

int main()
{
	PromLogSetLevel(TRACE_LEVEL_INFO);

	PromLogInfo(TEXT("Write info message"));

	PromLogError(TEXT("Write error message"));

	PromLogMsg(TEXT("Write message"));

	return 0;
}