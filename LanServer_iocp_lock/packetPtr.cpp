#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include "dump.h"

#include "log.h"
#include "ObjectFreeListTLS.h"
#include "lockFreeQueue.h"
#include "protocolBuffer.h"
#include "SimpleProfiler.h"

#include "packetPtr.h"

CObjectFreeListTLS<stPacket>* CPacketPtr::
	_freeList = new CObjectFreeListTLS<stPacket> (HeapCreate(0,0,0), false, false);

//#define PACKET_PTR_REF_AUTO

void CPacketPtr::erase(){
	
	//_freeList->freeObject(_packet);
	delete _packet;
	_packet = nullptr;
	
}

CPacketPtr::~CPacketPtr(){
	
	#ifndef PACKET_PTR_REF_AUTO
		return ;
	#endif
	
	int ref = InterlockedDecrement((LONG*)&_packet->_ref);
	if(ref == 0){
		delete _packet;
		//_freeList->freeObject(_packet);
		_packet = nullptr;
	}

}

void CPacketPtr::incRef(){

	#ifdef PACKET_PTR_REF_AUTO
		return ;
	#endif
	
	int ref = InterlockedIncrement((LONG*)&_packet->_ref);
}

void CPacketPtr::decRef(){
	
	#ifdef PACKET_PTR_REF_AUTO
		//this->~CPacketPtr();
		return ;
	#endif
	
	int ref = InterlockedDecrement((LONG*)&_packet->_ref);
	if(ref == 0){
		//printf("release\n");
		delete _packet;
		//_freeList->freeObject(_packet);
		_packet = nullptr;
	}

}

bool CPacketPtr::checkBufferSize(int size){

	if(_packet->_buffer.getFreeSize() < size){
		return false;
	}

	return true;

}

unsigned __int64 CPacketPtr::getPacketPoolUsage(){

	return CPacketPtr::_freeList->getCapacity();

}

void CPacketPtr::popData(int size, unsigned char* buf){

	_packet->_buffer.popData(size, buf);

}

void CPacketPtr::putData(int size, unsigned char* buf){
	
	_packet->_buffer.putData(size, (char*)buf);

}

CPacketPtr& CPacketPtr::operator<<(char data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(unsigned char data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(wchar_t data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(short data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(unsigned short data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(int data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(unsigned int data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(__int64 data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(unsigned __int64 data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(float data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator<<(double data){
	_packet->_buffer << data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(char& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(unsigned char& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(wchar_t& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(short& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(unsigned short& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(int& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(unsigned int& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(__int64& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(unsigned __int64& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(float& data){
	_packet->_buffer >> data;
	return *this;
}

CPacketPtr& CPacketPtr::operator>>(double& data){
	_packet->_buffer >> data;
	return *this;
}