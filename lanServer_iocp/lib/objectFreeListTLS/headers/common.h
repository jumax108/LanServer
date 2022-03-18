#pragma once

//#define OBJECT_FREE_LIST_TLS_SAFE

namespace objectFreeListTLS{
	// T type data 주소에 이 값을 더하면 node 주소가 됩니다.
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		constexpr __int64 _dataToNodePtr = -8;
	#else
		constexpr __int64 _dataToNodePtr = 0;
	#endif

	// 하나의 청크에서 사용할 T type 노드의 수 입니다.
	constexpr int _chunkSize = 2;

	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		constexpr unsigned __int64 UNDERFLOW_CHECK_VALUE = 0xAABBCCDDDDCCBBAA;
		constexpr unsigned __int64 OVERFLOW_CHECK_VALUE  = 0xFFEEDDCCCCDDEEFF;
	#endif
};