#pragma once

struct stHeader{
	unsigned short size;
};

constexpr int MAX_PACKET = 100;

constexpr unsigned __int64 _sessionIndexMask = 0x000000000000FFFF;
constexpr unsigned __int64 _sessionAllocCntMask = 0xFFFFFFFFFFFF0000;