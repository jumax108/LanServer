#include "headers/packetPointer_LanServer.h"

void CPacketPtr_Lan::setHeader(){

	stHeader header;
	header.size = _packet->_buffer.getUsedSize() - sizeof(stHeader);

	printf("header size: %d\n", header.size);
	memcpy(_packet->_buffer.getBufStart(), &header, sizeof(stHeader));

}

CPacketPtr_Lan::CPacketPtr_Lan(){

	this->_packet->_buffer.moveRear(sizeof(stHeader));

}