#include "headers/packetPointer_LanServer.h"


void CPacketPtr_Lan::setHeader(void* args){

	stHeader header;
	header.size = _packet->_buffer.getUsedSize();

	memcpy(_packet->_buffer.getBufStart(), &header, sizeof(stHeader));

}