#pragma once

class CProtocolBuffer {

public:

	CProtocolBuffer(unsigned int size = 500);
   ~CProtocolBuffer();

#pragma region("operator<<")
	CProtocolBuffer* operator<<(char data);
	CProtocolBuffer* operator<<(unsigned char data);

	CProtocolBuffer* operator<<(wchar_t data);
	CProtocolBuffer* operator<<(short data);
	CProtocolBuffer* operator<<(unsigned short data);

	CProtocolBuffer* operator<<(int data);
	CProtocolBuffer* operator<<(unsigned int data);

	CProtocolBuffer* operator<<(const __int64 data);
	CProtocolBuffer* operator<<(const unsigned __int64 data);

	CProtocolBuffer* operator<<(float data);
	CProtocolBuffer* operator<<(const double data);
#pragma endregion

#pragma region("operator>>")
	CProtocolBuffer* operator>>(char& data);
	CProtocolBuffer* operator>>(unsigned char& data);

	CProtocolBuffer* operator>>(wchar_t& data);
	CProtocolBuffer* operator>>(short& data);
	CProtocolBuffer* operator>>(unsigned short& data);

	CProtocolBuffer* operator>>(int& data);
	CProtocolBuffer* operator>>(unsigned int& data);

	CProtocolBuffer* operator>>(__int64& data);
	CProtocolBuffer* operator>>(unsigned __int64& data);

	CProtocolBuffer* operator>>(float& data);
	CProtocolBuffer* operator>>(double& data);
#pragma endregion

	void putData(unsigned int size, const char* data);
	void putDataW(unsigned int size, const wchar_t* data);
	bool popData(unsigned int size, unsigned char* data);
	bool popDataW(unsigned int size, wchar_t* data);

	inline int getUsedSize(){
		return _rear - _front;
	}
	inline int getFreeSize(){
		return _capacity - _rear;
	}

	inline char* getRearPtr(){
		return &_buffer[_rear];
	}

	inline char* getFrontPtr(){
		return &_buffer[_front];
	}

	inline void moveRear(unsigned int size){
		_rear += size;
	}
	inline void moveFront(unsigned int size){
		_front += size;
	}
	
	inline void frontSetZero(){
		_front = 0;
	}

	inline void clear(){
		_front = 0;
		_rear = 0;
	}

	inline char* getBufStart(){
		return _buffer;
	}
	
	void resize(unsigned int cap, bool writeFile = true);

protected:

public:	char* _buffer;
	unsigned int _capacity;
	unsigned int _front;
	unsigned int _rear;
	static int _cnt;


};