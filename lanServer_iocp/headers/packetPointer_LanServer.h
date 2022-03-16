#pragma once

#include "packetPointer/headers/packetPointer.h"
#pragma comment(lib, "lib/packetPointer/packetPointer")

#include "common.h"

class CPacketPtr_Lan: public CPacketPointer{
public:

	CPacketPtr_Lan();

	virtual void setHeader();
	virtual void incoding(){}
	virtual void decoding(){}

};
