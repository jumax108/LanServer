#pragma once

#include "packetPointer/headers/packetPointer.h"
#pragma comment(lib, "lib/packetPointer/packetPointer")

#include "common.h"

class CPacketPtr_Lan: public CPacketPointer{
public:

	CPacketPtr_Lan();
	CPacketPtr_Lan(CPacketPtr_Lan& ptr);

	virtual void setHeader();
	virtual void incoding(){}
	virtual void decoding(){}

private:

	#if defined(PACKET_PTR_LAN_DEBUG)
		void* returnAdr;

		static stPacket* arr[65536];
		static int arrIndex;
	#endif

};
