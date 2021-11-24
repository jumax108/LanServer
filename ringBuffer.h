#pragma once

#define push(size, buffer) pushData(size, buffer, __LINE__, __FILEW__)

class CRingBuffer {

public:

	CRingBuffer(unsigned int capacity);
	~CRingBuffer();

	bool pushData(unsigned int size, const char* buffer, int line, const wchar_t* sourceFileName);
	bool pop(unsigned int size);
	bool front(unsigned int size, char* buffer);

	inline int capacity() {
		return _capacity;
	}

	inline int rear() {
		return _rear;
	}

	inline int front() {
		return _front;
	}

	char* getDirectPush();
	char* getDirectFront();

	unsigned int getUsedSize();
	unsigned int getFreeSize();

	unsigned int getDirectFreeSize();
	unsigned int getDirectUsedSize();

	inline void pushLock() {
		AcquireSRWLockExclusive(&pushSRW);
	}
	inline void pushUnlock() {
		ReleaseSRWLockExclusive(&pushSRW);
	}

	inline void popLock() {
		AcquireSRWLockExclusive(&popSRW);
	}
	inline void popUnlock() {
		ReleaseSRWLockExclusive(&popSRW);
	}

	bool moveFront(unsigned int);
	bool moveRear(unsigned int);

	char* getBufferStart(){return _buffer;}

	char* getRearPtr(){return &_buffer[_rear];}
	char* getFrontPtr(){return &_buffer[_front];}

private:
	
	void resize(int size, int line, const wchar_t* sourceFileName);

private:

	SRWLOCK pushSRW;
	SRWLOCK popSRW;

	char* _buffer;

	unsigned int _capacity;

	// capacity + 1, 실제로 할당받은 객체 수
	unsigned int _actualCap;

	unsigned int _rear;
	unsigned int _front;

	wchar_t fileName[35];

};