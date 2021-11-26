#include "log.h"

void CLog::operator()(const wchar_t* fileName, LOG_GROUP printGroup, const wchar_t* format, ...){

	if((unsigned __int64)(_printGroup & printGroup) == 0){
		return ;
	}

	size_t fileNameLen = wcslen(fileName);

	size_t fullFileNameLen = fileNameLen + _directoryLen + 2;
	wchar_t* fullFileName = new wchar_t[fullFileNameLen];
				
	StringCchPrintf(fullFileName, fullFileNameLen, L"%s\\%s", _directory, fileName);


	lock(); {
		
		FILE* file;

		_wfopen_s(&file, fullFileName, L"a");
		
		va_list vaList;
		va_start(vaList, format);

		vfwprintf_s(file, format, vaList);
		fwprintf_s(file, L"\n");

		va_end(vaList);

		fclose(file);

		delete fullFileName;

	} unlock();

}

void CLog::setPrintGroup(LOG_GROUP printGroup){

	_printGroup = printGroup;

}

int CLog::setDirectory(const wchar_t* dirName){

	_directory = (wchar_t*)dirName;

	_directoryLen = wcslen(dirName);

	return _wmkdir(dirName);

}

CLog::CLog(){

	_directory = nullptr;
	InitializeSRWLock(&_lock);

}

void CLog::lock(){
	AcquireSRWLockExclusive(&_lock);
}

void CLog::unlock(){
	ReleaseSRWLockExclusive(&_lock);
}