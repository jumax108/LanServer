#include <stdio.h>
#include <Windows.h>
#include <time.h>

#include "protocolBuffer.h"

int CProtocolBuffer::_cnt = 0;

CProtocolBuffer::CProtocolBuffer(unsigned int size) {

	_capacity = size;

	_front = 0;
	_rear = 0;
	
	if(size == 0){
		return ;
	}

	_buffer = (char*)malloc(_capacity);
	ZeroMemory(_buffer, _capacity);
	
}

CProtocolBuffer::~CProtocolBuffer() {

	free(_buffer);

}

void CProtocolBuffer::resize(unsigned int cap, bool writeFile) {

	char* newBuffer = (char*)malloc(cap);
	
	memcpy(newBuffer, _buffer, _capacity);

	//printf("freeBuffer: %I64x\n",_buffer);
	free(_buffer);

	_buffer = newBuffer;

	wchar_t wcsTime[26] = {0,};
	time_t nowTime;
	time(&nowTime);
	tm now;
	localtime_s(&now, &nowTime);
	wchar_t fileName[50] = {0,};
	int cnt = InterlockedIncrement((LONG*)&_cnt);
	swprintf_s(fileName, 50, L"resizeLog_%04d%02d%02d_%02d%02d%02d_%d.txt", now.tm_year+1900, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, cnt);

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

	_capacity = cap;
}

void CProtocolBuffer::putData(unsigned int size, const char* data) {

	if (size + _rear >= _capacity) {
		resize(_capacity + size);
	}

	memcpy(_buffer + _rear, data, size);
	_rear += size;

}
void CProtocolBuffer::putDataW(unsigned int size, const wchar_t* data) {

	if (size * 2 + _rear >= _capacity ) {
		resize(_capacity + size * 2);
	}

	memcpy(_buffer + _rear, data, (unsigned __int64)size * 2);
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

	memcpy(data, _buffer + _front, (unsigned __int64)size * 2);
	_front += size * 2;

	return true;
}

#pragma region("operator<<")
CProtocolBuffer* CProtocolBuffer::operator<<(char data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned char data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(wchar_t data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(wchar_t*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(short data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(short*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned short data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(unsigned short*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(int data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(int*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(unsigned int data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(unsigned int*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const __int64 data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(__int64*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const unsigned __int64 data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(unsigned __int64*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(float data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(float*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}

CProtocolBuffer* CProtocolBuffer::operator<<(const double data) {
	if (_rear + sizeof(data) > _capacity) {
		resize(_capacity + sizeof(data));
	}
	*(double*)(_buffer + _rear) = data;
	_rear += sizeof(data);
	return this;
}
#pragma endregion

#pragma region("operator>>")

CProtocolBuffer* CProtocolBuffer::operator>>(char& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned char& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(wchar_t& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(wchar_t*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(short& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(short*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned short& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(unsigned short*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(int& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(int*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned int& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(unsigned int*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(__int64& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(_int64*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(unsigned __int64& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(unsigned __int64*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(float& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(float*)(_buffer + _front);
	_front += sizeof(data);

}

CProtocolBuffer* CProtocolBuffer::operator>>(double& data) {

	if (_rear - _front < sizeof(data)) {
		return this;
	}
	data = *(double*)(_buffer + _front);
	_front += sizeof(data);

}
#pragma endregion