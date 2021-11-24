#include <stdio.h>
#include <Windows.h>
#include <time.h>

#include "protocolBuffer.h"

CProtocolBuffer::CProtocolBuffer(unsigned int size) {

	_capacity = size;

	_front = 0;
	_rear = 0;

	_buffer = (char*)malloc(_capacity);
	ZeroMemory(_buffer, _capacity);

}

CProtocolBuffer::~CProtocolBuffer() {

	free(_buffer);

}

void CProtocolBuffer::resize() {

	char* newBuffer = (char*)malloc(_capacity * 2);
	
	memcpy(newBuffer, _buffer, _rear);

	free(_buffer);

	_buffer = newBuffer;

	wchar_t wcsTime[26] = {0,};
	time_t nowTime;
	time(&nowTime);
	tm now;
	localtime_s(&now, &nowTime);
	wchar_t fileName[50] = {0,};
	swprintf_s(fileName, 50, L"resizeLog_%04d%02d%02d_%02d%02d%02d.txt", now.tm_year+1900, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);

	FILE* resizeLog;
	_wfopen_s(&resizeLog, fileName, L"w");

	fwprintf_s(resizeLog, L"protocol buffer resized\n\nbefore size: %d\nafter size: %d\n\nbuffer start in hex\n", _capacity, _capacity * 2);
	
	char* bufEnd = _buffer + _capacity;
	for (char* bufIter = _buffer; bufIter != bufEnd; ++bufIter) {
		if ((bufEnd - bufIter) % 10 == 0) {
			fwprintf_s(resizeLog, L"\n");
		}
		fwprintf_s(resizeLog, L"%02X ", *bufIter);
	}

	fclose(resizeLog);

	_capacity = _capacity * 2;
}

void CProtocolBuffer::putData(unsigned int size, const char* data) {

	if (size > _capacity - _rear) {
		resize();
	}

	memcpy(_buffer + _rear, data, size);
	_rear += size;

}
void CProtocolBuffer::putDataW(unsigned int size, const wchar_t* data) {

	if (size * 2 > _capacity - _rear) {
		resize();
	}

	memcpy(_buffer + _rear, data, size * 2);
	_rear += size * 2;

}

bool CProtocolBuffer::popData(unsigned int size, unsigned char* data) {

	if (_rear - _front < size) {
		return false;
	}

	memcpy(data, _buffer + _front, size);
	_front += size;

	return true;

}

bool CProtocolBuffer::popDataW(unsigned int size, wchar_t* data) {
	if (_rear - _front < size * 2) {
		return false;
	}

	memcpy(data, _buffer + _front, size * 2);
	_front += size * 2;

	return true;
}

int CProtocolBuffer::getUsedSize() {
	return _rear - _front;
}

int CProtocolBuffer::getFreeSize() {
	return _capacity - _rear;
}

#pragma region("operator<<")
CProtocolBuffer* CProtocolBuffer::operator<<(char data) {
	if (_rear + 1 > _capacity) {
		resize();
	}
	*(_buffer + _rear) = data;
	_rear += 1;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned char data) {
	if (_rear + 1 > _capacity) {
		resize();
	}
	*(_buffer + _rear) = data;
	_rear += 1;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(wchar_t data) {
	if (_rear + 2 > _capacity) {
		resize();
	}
	*(wchar_t*)(_buffer + _rear) = data;
	_rear += 2;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(short data) {
	if (_rear + 2 > _capacity) {
		resize();
	}
	*(short*)(_buffer + _rear) = data;
	_rear += 2;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned short data) {
	if (_rear + 2 > _capacity) {
		resize();
	}
	*(unsigned short*)(_buffer + _rear) = data;
	_rear += 2;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(int data) {
	if (_rear + 4 > _capacity) {
		resize();
	}
	*(int*)(_buffer + _rear) = data;
	_rear += 4;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned int data) {
	if (_rear + 4 > _capacity) {
		resize();
	}
	*(unsigned int*)(_buffer + _rear) = data;
	_rear += 4;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const __int64& data) {
	if (_rear + 8 > _capacity) {
		resize();
	}
	*(__int64*)(_buffer + _rear) = data;
	_rear += 8;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const unsigned __int64& data) {
	if (_rear + 8 > _capacity) {
		resize();
	}
	*(unsigned __int64*)(_buffer + _rear) = data;
	_rear += 8;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(float data) {
	if (_rear + 4 > _capacity) {
		resize();
	}
	*(float*)(_buffer + _rear) = data;
	_rear += 4;
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const double& data) {
	if (_rear + 8 > _capacity) {
		resize();
	}
	*(double*)(_buffer + _rear) = data;
	_rear += 8;
	return this;
}
#pragma endregion

#pragma region("operator>>")

CProtocolBuffer* CProtocolBuffer::operator>>(char& data) {

	if (_rear - _front < 1) {
		return this;
	}
	data = *(_buffer + _front);
	_front += 1;

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned char& data) {

	if (_rear - _front < 1) {
		return this;
	}
	data = *(_buffer + _front);
	_front += 1;

}

CProtocolBuffer* CProtocolBuffer::operator>>(wchar_t& data) {

	if (_rear - _front < 2) {
		return this;
	}
	data = *(wchar_t*)(_buffer + _front);
	_front += 2;

}

CProtocolBuffer* CProtocolBuffer::operator>>(short& data) {

	if (_rear - _front < 2) {
		return this;
	}
	data = *(short*)(_buffer + _front);
	_front += 2;

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned short& data) {

	if (_rear - _front < 2) {
		return this;
	}
	data = *(unsigned short*)(_buffer + _front);
	_front += 2;

}

CProtocolBuffer* CProtocolBuffer::operator>>(int& data) {

	if (_rear - _front < 4) {
		return this;
	}
	data = *(int*)(_buffer + _front);
	_front += 4;

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned int& data) {

	if (_rear - _front < 4) {
		return this;
	}
	data = *(unsigned int*)(_buffer + _front);
	_front += 4;

}

CProtocolBuffer* CProtocolBuffer::operator>>(__int64& data) {

	if (_rear - _front < 8) {
		return this;
	}
	data = *(__int64*)(_buffer + _front);
	_front += 8;

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned __int64& data) {

	if (_rear - _front < 8) {
		return this;
	}
	data = *(unsigned __int64*)(_buffer + _front);
	_front += 8;

}

CProtocolBuffer* CProtocolBuffer::operator>>(float& data) {

	if (_rear - _front < 4) {
		return this;
	}
	data = *(float*)(_buffer + _front);
	_front += 4;

}

CProtocolBuffer* CProtocolBuffer::operator>>(double& data) {

	if (_rear - _front < 8) {
		return this;
	}
	data = *(double*)(_buffer + _front);
	_front += 8;

}


#pragma endregion

char* CProtocolBuffer::getRearPtr() {
	return &_buffer[_rear];
}

char* CProtocolBuffer::getFrontPtr() {
	return &_buffer[_front];
}

void CProtocolBuffer::moveRear(unsigned int size) {
	_rear += size;
}

void CProtocolBuffer::moveFront(unsigned int size) {
	_front += size;
}

void CProtocolBuffer::frontSetZero(){
	_front = 0;
}