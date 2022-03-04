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

		// ��Ŷ ������ �ʱ�ȭ
		_packet->_buffer.clear();

		// ��Ŷ ��� ���� Ȯ��
		_packet->_buffer.moveRear(sizeof(stHeader));

	}

	virtual void setHeader(){

		int payloadSize = _packet->_buffer.getUsedSize() - sizeof(stHeader);

		stHeader* header = (stHeader*)_packet->_buffer.getBufStart();
		header->size = payloadSize;

	}

};