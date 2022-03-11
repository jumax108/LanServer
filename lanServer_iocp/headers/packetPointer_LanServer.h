#pragma once

#include "packetPointer/headers/packetPointer.h"
#pragma comment(lib, "lib/packetPointer/packetPointer")

#include "common.h"

class CPacketPtr_Lan: public CPacketPointer{
public:
	virtual void setHeader(void* args);

};
