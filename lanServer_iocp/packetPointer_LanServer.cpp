
#include "objectFreeListTLS/headers/objectFreeListTLS.h"
#include "headers/packetPointer_LanServer.h"

void CPacketPtr_Lan::setHeader(){
	
	CProtocolBuffer* buffer = &_packet->_buffer;

	stHeader* header = (stHeader*)buffer->getBufStart();
	header->size = buffer->getUsedSize() - sizeof(stHeader);

}

CPacketPtr_Lan::CPacketPtr_Lan(){

	this->_packet->_buffer.moveRear(sizeof(stHeader));

}