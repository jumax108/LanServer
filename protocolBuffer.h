#pragma once

class CProtocolBuffer {

public:

	CProtocolBuffer(unsigned int size);
   ~CProtocolBuffer();

#pragma region("operator<<")
	CProtocolBuffer* operator<<(char data);
	CProtocolBuffer* operator<<(unsigned char data);

	CProtocolBuffer* operator<<(wchar_t data);
	CProtocolBuffer* operator<<(short data);
	CProtocolBuffer* operator<<(unsigned short data);

	CProtocolBuffer* operator<<(int data);
	CProtocolBuffer* operator<<(unsigned int data);

	CProtocolBuffer* operator<<(const __int64& data);
	CProtocolBuffer* operator<<(const unsigned __int64& data);

	CProtocolBuffer* operator<<(float data);
	CProtocolBuffer* operator<<(const double& data);
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

	int getUsedSize();
	int getFreeSize();

	char* getRearPtr();
	char* getFrontPtr();

	void moveRear(unsigned int size);
	void moveFront(unsigned int size);

	void frontSetZero();

private:

	char* _buffer;
	unsigned int _capacity;
	unsigned int _front;
	unsigned int _rear;

	void resize();

};