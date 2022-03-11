#pragma once

#include "objectFreeList\headers\objectFreeList.h"

#include "common.h"

#if defined(OBJECT_FREE_LIST_TLS_SAFE)
	#define allocObject() _allocObject(__FILEW__, __LINE__)
	#define freeObject(ptr) _freeObject(ptr, __FILEW__, __LINE__)
#else
	#define allocObject() _allocObject()
	#define freeObject(ptr) _freeObject(ptr)
#endif

template <typename T>
struct stAllocChunk;

template <typename T>
struct stAllocTlsNode;

template <typename T>
class CObjectFreeListTLS{

public:

	CObjectFreeListTLS(bool runConstructor, bool runDestructor);
	~CObjectFreeListTLS();

	T*	 _allocObject(
		#if defined(OBJECT_FREE_LIST_TLS_SAFE)
			const wchar_t*, int
		#endif
	);
	void _freeObject (T* object
		#if defined(OBJECT_FREE_LIST_TLS_SAFE)
			,const wchar_t*, int
		#endif
	);

	unsigned int getCapacity();
	unsigned int getUsedCount();
	
private:
	
	// 각 스레드에서 활용할 청크를 들고있는 tls의 index
	unsigned int _allocChunkTlsIdx;

	// 모든 스레드가 공통적으로 접근하는 free list 입니다.
	// 이곳에서 T type의 node를 큰 덩어리로 들고옵니다.
	CObjectFreeList<stAllocChunk<T>>* _centerFreeList;
			
	// T type에 대한 생성자 호출 여부
	bool _runConstructor;

	// T type에 대한 소멸자 호출 여부
	bool _runDestructor;

};

template <typename T>
CObjectFreeListTLS<T>::CObjectFreeListTLS(bool runConstructor, bool runDestructor){

	
	///////////////////////////////////////////////////////////////////////
	// 중앙 처리용 freeList 생성
	_centerFreeList = new CObjectFreeList<stAllocChunk<T>>(runConstructor, runDestructor);
	///////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////
	// tls 할당 받음
	_allocChunkTlsIdx = TlsAlloc();
	if(_allocChunkTlsIdx == TLS_OUT_OF_INDEXES){
		CDump::crash();
	}
	///////////////////////////////////////////////////////////////////////

	_runConstructor = runConstructor;
	_runDestructor	= runDestructor;

}

template <typename T>
CObjectFreeListTLS<T>::~CObjectFreeListTLS(){
	TlsFree(_allocChunkTlsIdx);
	delete _centerFreeList;
}

template <typename T>
typename T* CObjectFreeListTLS<T>::_allocObject(
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		const wchar_t* fileName,
		int line
	#endif
){

	///////////////////////////////////////////////////////////////////////
	// 청크 획득
	stAllocChunk<T>* chunk = (stAllocChunk<T>*)TlsGetValue(_allocChunkTlsIdx);
	if(chunk == nullptr){
		chunk = _centerFreeList->allocObject();
		chunk->init();
		TlsSetValue(_allocChunkTlsIdx, chunk);
	}
	///////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////
	// 청크에서 노드 획득 & 초기화
	stAllocTlsNode<T>* allocNode = chunk->_allocNode;
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		allocNode->_allocated = true;
		allocNode->_allocSourceFileName = fileName;
		allocNode->_allocLine = line;
	#endif
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// 노드에서 T type 포인터 획득
	T* allocData = &allocNode->_data;

	if(_runConstructor == true){
	//	new (allocData) T();
	}
	///////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////
	// 다음 노드 바라보기
	chunk->_allocNode += 1;
	///////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////
	// 청크를 모두 사용했다면 새로 할당받기
	if(chunk->_allocNode == chunk->_nodeEnd){
		chunk = _centerFreeList->allocObject();
		chunk->init();
		TlsSetValue(_allocChunkTlsIdx, chunk);
	}
	///////////////////////////////////////////////////////////////////////

	return allocData;

}

template <typename T>
void CObjectFreeListTLS<T>::_freeObject(T* object
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		,const wchar_t* fileName,
		int line
	#endif
){
	
	///////////////////////////////////////////////////////////////////////
	// 할당했던 노드 획득
	stAllocTlsNode<T>* allocatedNode = (stAllocTlsNode<T>*)((unsigned __int64)object + objectFreeListTLS::_dataToNodePtr);
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// object 정상 사용 여부 체크
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		if(allocatedNode->_underflowCheck != objectFreeListTLS::UNDERFLOW_CHECK_VALUE){
			// underflow
			CDump::crash();
		}
		if(allocatedNode->_overflowCheck != objectFreeListTLS::OVERFLOW_CHECK_VALUE){
			// overflow
			CDump::crash();
		}
		if(allocatedNode->_allocated == false){
			// 할당되지 않은 노드 해제 시도
			CDump::crash();
		}
	#endif
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// 해제 요청한 소스 파일 & 라인 저장
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		allocatedNode->_freeSourceFileName = fileName;
		allocatedNode->_freeLine = line;
	#endif
	///////////////////////////////////////////////////////////////////////

	
	///////////////////////////////////////////////////////////////////////
	// 소멸자 호출
	if(_runDestructor == true){
	//	object->~T();
	}
	///////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////
	// 노드에서 청크 획득
	stAllocChunk<T>* chunk = allocatedNode->_afflicatedChunk;
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// 청크의 모든 요소가 사용 완료(할당 후 반환)되었다면 청크를 반환
	int leftFreeCnt = InterlockedDecrement((LONG*)&chunk->_leftFreeCnt);
	if(leftFreeCnt == 0){
		chunk->_leftFreeCnt = chunk->_nodeNum;
		chunk->_allocNode = chunk->_nodes;
		_centerFreeList->freeObject(chunk);
	}
	///////////////////////////////////////////////////////////////////////
}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getUsedCount(){
	
	return _centerFreeList->getUsedCount();

}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getCapacity(){
	
	return _centerFreeList->getCapacity();

}

template <typename T>
struct stAllocTlsNode{
	
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		unsigned __int64 _underflowCheck;
	#endif

	T _data;

	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		unsigned __int64 _overflowCheck;

		const wchar_t* _allocSourceFileName;
		const wchar_t*  _freeSourceFileName;

		int _allocLine;
		int  _freeLine;

		bool _allocated;

	#endif

	// 노드가 소속되어있는 청크
	stAllocChunk<T>* _afflicatedChunk;

	void init(stAllocChunk<T>* afflicatedChunk);
};

template <typename T>
void stAllocTlsNode<T>::init(stAllocChunk<T>* afflicatedChunk){
	#if defined(OBJECT_FREE_LIST_TLS_SAFE)
		_underflowCheck = objectFreeListTLS::UNDERFLOW_CHECK_VALUE;
		_overflowCheck  = objectFreeListTLS::OVERFLOW_CHECK_VALUE;

		_allocSourceFileName = nullptr;
		_freeSourceFileName = nullptr;

		_allocLine = 0;
		_freeLine = 0;
	
		_allocated = false;
	#endif

	_afflicatedChunk = afflicatedChunk;
}


template <typename T>
struct stAllocChunk{
		
public:

	int _leftFreeCnt;
	stAllocTlsNode<T>* _allocNode;

	stAllocTlsNode<T> _nodes[objectFreeListTLS::_chunkSize];	
	stAllocTlsNode<T>* _nodeEnd;

	int _nodeNum;

	void init();

};

template<typename T>
void stAllocChunk<T>::init(){
	
	_nodeNum = objectFreeListTLS::_chunkSize;

	_allocNode = _nodes;
	_nodeEnd   = _nodes + _nodeNum;

	_leftFreeCnt = _nodeNum;

	for(int nodeCnt = 0; nodeCnt < _nodeNum; ++nodeCnt){
		_nodes[nodeCnt].init(this);
	}
}