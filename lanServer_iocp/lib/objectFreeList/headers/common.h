#pragma once


//#define OBJECT_FREE_LIST_DEBUG
#define USE_OWN_HEAP

namespace objectFreeList_stack_lock {

	// T type data 주소에 이 값을 더하면 node 주소가 됩니다.
	#if defined(OBJECT_FREE_LIST_TLS_DEBUG)
		constexpr __int64 DATA_TO_NODE_PTR = -8;
	#else
		constexpr __int64 DATA_TO_NODE_PTR = 0;
	#endif

}