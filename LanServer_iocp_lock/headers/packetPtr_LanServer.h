#pragma once

#include "packetPtr.h"
#include "common.h"

class CPacketPtrLan : public CPacketPtr{
public:
	CPacketPtrLan(){
	
		//_packet = _freeList->allocObject();
		_packet = new stPacket;
		_packet->_incoded = false;
		_packet->_ref = 1;

		// 패킷 데이터 초기화
		_packet->_buffer.clear();

		// 패킷 헤더 공간 확보
		_packet->_buffer.moveRear(sizeof(stHeader));

	}

	virtual void setHeader(){

		int payloadSize = _packet->_buffer.getUsedSize() - sizeof(stHeader);

		stHeader* header = (stHeader*)_packet->_buffer.getBufStart();
		header->size = payloadSize;

	}

};