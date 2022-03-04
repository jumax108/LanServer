#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include "dump.h"

long CDump::_dumpCnt;

CDump::CDump(){

	_dumpCnt = 0;

	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = myInvalidParameterHandler;

	oldHandler = _set_invalid_parameter_handler(newHandler);
	
	_CrtSetReportHook(customReportHook);

	_set_purecall_handler(myPureCallHandler);

	setHandlerDump();

}

void CDump::crash(){
	int *ptr = nullptr;
	*ptr = 0;
}

long __stdcall CDump::myExceptionFilter(PEXCEPTION_POINTERS exceptionPointer){
	
	SYSTEMTIME nowTime;

	wchar_t fileName[1000];
	
	GetLocalTime(&nowTime);

	wsprintfW(fileName, L"Dump_%d%02d%02d_%02d.%02d.%02d.%d.dmp", nowTime.wYear, nowTime.wMonth, nowTime.wDay, nowTime.wHour, nowTime.wMinute, nowTime.wSecond, _dumpCnt);

	HANDLE dumpFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(dumpFile != INVALID_HANDLE_VALUE){
		
		_MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInformation;

		miniDumpExceptionInformation.ThreadId = GetCurrentThreadId();
		miniDumpExceptionInformation.ExceptionPointers = exceptionPointer;
		miniDumpExceptionInformation.ClientPointers = true;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithFullMemory, &miniDumpExceptionInformation, NULL, NULL);

		CloseHandle(dumpFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void CDump::setHandlerDump(){
	SetUnhandledExceptionFilter(myExceptionFilter);
}

void CDump::myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t resvered){
	crash();
}

int CDump::customReportHook(int repostType, char* msg, int* returnValue){
	crash();
	return true;
}

void CDump::myPureCallHandler(){
	crash();
}