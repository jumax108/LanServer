
#include "objectFreeListTLS/headers/objectFreeListTLS.h"
#include "headers/packetPointer_LanServer.h"

#if defined(PACKET_PTR_LAN_DEBUG)

	stPacket* CPacketPtr_Lan::arr[65536] = {0,};
	int CPacketPtr_Lan::arrIndex = 0;

#endif

void CPacketPtr_Lan::setHeader() {

	CProtocolBuffer* buffer = &_packet->_buffer;

	stHeader* header = (stHeader*)buffer->getBufStart();
	header->size = buffer->getUsedSize() - sizeof(stHeader);

} 

CPacketPtr_Lan::CPacketPtr_Lan() {

	this->_packet->_buffer.moveRear(sizeof(stHeader));

	#if defined(PACKET_PTR_LAN_DEBUG)
		returnAdr = _ReturnAddress();

		unsigned short index = InterlockedIncrement((LONG*)&arrIndex) - 1;
		arr[index] = _packet;
	#endif
}

CPacketPtr_Lan::CPacketPtr_Lan(CPacketPtr_Lan& ptr)
	:CPacketPointer(ptr){
	
	#if defined(PACKET_PTR_LAN_DEBUG)
		returnAdr = ptr.returnAdr;

		unsigned short index = InterlockedIncrement((LONG*)&arrIndex) - 1;
		arr[index] = _packet;
	#endif
}