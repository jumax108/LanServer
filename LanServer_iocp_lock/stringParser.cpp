#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "StringParser.h"

CStringParser::CStringParser(const WCHAR* fileName) {

	_nameSpace = nullptr;
	_data = nullptr;
	_dataEnd = nullptr;
	__int64 fileNameLen = (int)wcslen(fileName);
	_fileName = (WCHAR*)malloc(sizeof(WCHAR) * (fileNameLen + 1));
	if (_fileName == nullptr) {
		return;
	}
	_fileName[fileNameLen] = '\0';
	wmemcpy(_fileName, fileName, fileNameLen);

	readFile();

}

CStringParser::~CStringParser() {
	free(_data);
}

void CStringParser::readFile() {

	FILE* dataFile;
	_wfopen_s(&dataFile, _fileName, L"rb");


	fseek(dataFile, 0, SEEK_END);
	__int64 dataSize = ftell(dataFile);
	fseek(dataFile, 0, SEEK_SET);

	WCHAR checkLE;
	fread(&checkLE, sizeof(WCHAR), 1, dataFile);
	if (checkLE != 0xfeff) {
 		return;
	}

	WCHAR* buffer = (WCHAR*)malloc((dataSize + 1) * sizeof(WCHAR));
	buffer[dataSize] = '\0';

	size_t readSize = fread(buffer, sizeof(WCHAR), dataSize, dataFile);
	fclose(dataFile);
	buffer[readSize] = '\0';

	__int64 dataLen = wcslen(buffer);

	_data = (WCHAR*)malloc((dataLen + 1) * sizeof(WCHAR));
	_data[dataLen] = NULL;

	wmemcpy(_data, buffer, dataLen);
	_dataEnd = _data + dataLen;

	free(buffer);
}

void CStringParser::setNameSpace(const WCHAR* nameSpace) {

	__int64 len = (int)wcslen(nameSpace);

	free(_nameSpace);
	_nameSpace = (WCHAR*)malloc(sizeof(WCHAR) * (len + 1));
	_nameSpace[len] = '\0';

	wmemcpy(_nameSpace, nameSpace, len);

}

void CStringParser::initNameSpace() {
	free(_nameSpace);
	_nameSpace = nullptr;
}

bool isBlank(WCHAR in) {
	return in == *L" " || in == *L"\n" || in == 0x000d || in == *L"\t";
}

WCHAR* CStringParser::getNextWord(WCHAR* buf, WCHAR* startPoint) {

	startPoint = skipBlank(startPoint);
	WCHAR* endPoint = startPoint;
 	while (isBlank(*endPoint) == false && endPoint != _dataEnd) {
		endPoint += 1;
	}

	__int64 len = endPoint - startPoint;
	wmemcpy(buf, startPoint, len);
	buf[len] = '\0';

	return endPoint;
}

WCHAR* CStringParser::skipBlank(WCHAR* startPoint) {

	while (isBlank(*startPoint)) {
		startPoint += 1;
	}

	return startPoint;
}

WCHAR* CStringParser::findNameSpace(WCHAR* startPoint) {

	WCHAR* buf = (WCHAR*)malloc(sizeof(WCHAR) * 255);
	if (buf == nullptr) {
		return nullptr;
	}

	do {
		startPoint = getNextWord(buf, startPoint);
	} while (wcscmp(buf, _nameSpace) != 0);

	startPoint = getNextWord(buf, startPoint);

	free(buf);

	return startPoint;

}

WCHAR* CStringParser::getValueByKey(WCHAR* buf, const WCHAR* key) {

	WCHAR* nameSpaceStart = findNameSpace(_data);

	WCHAR* now = nameSpaceStart;

	WCHAR* tempBuf = (WCHAR*)malloc(sizeof(WCHAR) * 255);

	do {
		now = getNextWord(tempBuf, now);
		if (wcscmp(tempBuf, L"}") == 0){
			return nullptr;
		}
	} while (wcscmp(tempBuf, key) != 0);

	now = getNextWord(tempBuf, now);
	now = getNextWord(tempBuf, now);

	__int64 len = wcslen(tempBuf);

	wmemcpy(buf, tempBuf + 1, len - 2);
	buf[len - 2] = '\0';

	return now;

}