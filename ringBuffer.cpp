
#include <time.h>
#include <thread>
#include <Windows.h>
#include "ringBuffer.h"

#ifndef min
	#define min(a, b) ((a)>(b)?(b):(a))
#endif

#ifndef max
	#define max(a, b) ((a)>(b)?(a):(b))
#endif

CRingBuffer::CRingBuffer(unsigned int capacity) {

	InitializeSRWLock(&pushSRW);
	InitializeSRWLock(&popSRW);

	_capacity = capacity;
	_actualCap = _capacity + 1;
	_buffer = (char*)malloc((unsigned __int64)capacity + 1);
	ZeroMemory(_buffer, (unsigned __int64)capacity + 1);

	_rear = 0;
	_front = 0;

	tm timeStruct;
	__time64_t time;
	_time64(&time);
	_localtime64_s(&timeStruct, &time);

	swprintf_s(fileName, 35, L"%04d%02d%02d_%02d%02d%02d_ringBuffer_log.txt", 1900 + timeStruct.tm_year, timeStruct.tm_mon, timeStruct.tm_mday, timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec );
}

CRingBuffer::~CRingBuffer() {

	free(_buffer);
}

bool CRingBuffer::pushData(unsigned int size, const char* buffer, int line, const wchar_t* sourceFileName) {

		
	unsigned int freeSize = getFreeSize();
	if (freeSize < size) {
		resize(max(_capacity, freeSize + size), line, sourceFileName);
	}

	unsigned int rearTemp = _rear;

	do {

		unsigned int destBufferIndex = (rearTemp + size);
		if (destBufferIndex >= _actualCap) {
			destBufferIndex = _actualCap;
		}
		unsigned int copySize = destBufferIndex - rearTemp;
		memcpy(&_buffer[rearTemp], buffer, copySize);
		rearTemp = (rearTemp + copySize) % (_actualCap);
		buffer += copySize;
		size -= copySize;

	} while (size > 0);

	_rear = rearTemp;

	return true;

}

bool CRingBuffer::pop(unsigned int size) {

	unsigned int usedSize = getUsedSize();
	if (usedSize < size) {
		return false;
	}

	_front = (_front + size) % (_actualCap);

	return true;
}

bool CRingBuffer::front(unsigned int size, char* buffer) {

	unsigned int usedSize = getUsedSize();
	if (usedSize < size) {
		return false;
	}
	int copySize = 0;

	int frontTemp = _front;

	do {

		unsigned int destBufferIndex = (frontTemp + size);
		if (destBufferIndex >= _actualCap) {
			destBufferIndex = _actualCap;
		}
		copySize = destBufferIndex - frontTemp;
		memcpy(buffer, &_buffer[frontTemp], copySize);
		buffer += copySize;
		size -= copySize;
		frontTemp = (frontTemp + copySize) % _actualCap;

	} while (size > 0);

	return true;

}

unsigned int CRingBuffer::getFreeSize() {

	unsigned int front = _front;
	unsigned int rear = _rear;

	if(rear >= front){
		return _capacity - rear + front;
	} else {
		return front - rear - 1;
	}

}

unsigned int  CRingBuffer::getUsedSize() {

	unsigned int rear = _rear;
	unsigned int front = _front;

	if (rear >= front) {

		return rear - front;

	}
	else {

		return rear + _actualCap - front;

	}

}

char* CRingBuffer::getDirectPush() {
	return &_buffer[_rear];
}

char* CRingBuffer::getDirectFront() {
	return &_buffer[_front];
}

bool CRingBuffer::moveFront(unsigned int size) {

	_front = (_front + size) % (_actualCap);
	return true;
}

bool CRingBuffer::moveRear(unsigned int size) {

	_rear = (_rear + size) % (_actualCap);
	return true;

}

unsigned int CRingBuffer::getDirectFreeSize() {

	unsigned int front = _front;

	if (front > _rear) {
		return front - _rear - 1;
	}
	return _actualCap - _rear - (front == 0);
}

unsigned int CRingBuffer::getDirectUsedSize() {

	unsigned int rear = _rear;

	if (_front <= rear) {
		return rear - _front;
	}
	return _actualCap - _front;
}

void CRingBuffer::resize(int size, int line, const wchar_t* sourceFileName){

	char* newBuffer = (char*)malloc((unsigned __int64)size + 1);
	ZeroMemory(newBuffer, (unsigned __int64)size + 1);
	memcpy(newBuffer, _buffer, _capacity);

	free(_buffer);
	_buffer = newBuffer;

	FILE* logFile;
	_wfopen_s(&logFile, fileName, L"a");
	fwprintf_s(logFile, L"Resize, %d -> %d\nfile: %s\nline: %d\n\n", _capacity, size, sourceFileName, line);

	_capacity = size;
	_actualCap = _capacity + 1;

}