#ifndef __PACKET_PTR__

#define __PACKET_PTR__

static constexpr unsigned char constKey = 0x32;

struct stPacket{
	CProtocolBuffer _buffer;
	bool _incoded;
	alignas(64) int _ref;
};

class CPacketPtr{

public:

	inline CPacketPtr(CPacketPtr& ptr){
		_packet = ptr._packet;
		#ifdef PACKET_PTR_REF_AUTO
			InterlockedIncrement((long*)_ref);
		#endif
	}
	inline CPacketPtr& operator=(CPacketPtr& ptr){
		_packet = ptr._packet;
		#ifdef PACKET_PTR_REF_AUTO
			InterlockedIncrement((long*)_ref);
		#endif
		return *this;
	}

	CPacketPtr(){};
	~CPacketPtr();

	inline char* getBufStart(){ 
		return _packet->_buffer.getBufStart();
	}
	inline int getPacketSize(){
		return _packet->_buffer.getUsedSize();
	}
	inline char* getRearPtr(){
		return _packet->_buffer.getRearPtr();
	}
	inline char* getFrontPtr(){
		return _packet->_buffer.getFrontPtr();
	}
	inline void moveRear(int size){
		_packet->_buffer.moveRear(size);
	}
	inline void moveFront(int size){
		_packet->_buffer.moveFront(size);
	}

	bool checkBufferSize(int size);
	inline void setBufferSize(int size){	
		_packet->_buffer.resize(size, false);
	}
	
	void incRef();
	void decRef();

	virtual void incoding(){};
	virtual void decoding(){};
	virtual void setHeader(unsigned char randKey){};
	virtual void setHeader(){};

	void putData(int size, unsigned char* buf);
	void popData(int size, unsigned char* buf);
	
	void erase();

	static unsigned __int64 getPacketPoolUsage();

#pragma region("operator<<")
	CPacketPtr& operator<<(char data);
	CPacketPtr& operator<<(unsigned char data);

	CPacketPtr& operator<<(wchar_t data);
	CPacketPtr& operator<<(short data);
	CPacketPtr& operator<<(unsigned short data);

	CPacketPtr& operator<<(int data);
	CPacketPtr& operator<<(unsigned int data);

	CPacketPtr& operator<<(__int64 data);
	CPacketPtr& operator<<(unsigned __int64 data);

	CPacketPtr& operator<<(float data);
	CPacketPtr& operator<<(double data);
#pragma endregion

#pragma region("operator>>")
	CPacketPtr& operator>>(char& data);
	CPacketPtr& operator>>(unsigned char& data);

	CPacketPtr& operator>>(wchar_t& data);
	CPacketPtr& operator>>(short& data);
	CPacketPtr& operator>>(unsigned short& data);

	CPacketPtr& operator>>(int& data);
	CPacketPtr& operator>>(unsigned int& data);

	CPacketPtr& operator>>(__int64& data);
	CPacketPtr& operator>>(unsigned __int64& data);

	CPacketPtr& operator>>(float& data);
	CPacketPtr& operator>>(double& data);
#pragma endregion

	stPacket* _packet;

protected:

	static CObjectFreeListTLS<stPacket>* _freeList;

};

#endif
