#pragma once

struct stHeader{
	unsigned short size;
};

constexpr int MAX_PACKET = 100;

constexpr unsigned __int64 SESSION_INDEX_MASK = 0x000000000000FFFF;
constexpr unsigned __int64 SESSION_ALLOC_COUNT_MASK = 0xFFFFFFFFFFFF0000;