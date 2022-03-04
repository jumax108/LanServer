#ifndef __OBJECT_FREE_LIST_TLS__

#define __OBJECT_FREE_LIST_TLS__

//#define OBJECT_FREE_LIST_TLS_SAFE

#include "ObjectFreeList.h"
#include "SimpleProfiler.h"

// T type data 주소에 이 값을 더하면 node 주소가 됩니다.
static constexpr __int64 _dataToNodePtr = 0;

template <typename T>
struct stAllocChunk;

template <typename T>
struct stAllocTlsNode{
	
	T _data;

	// 노드가 소속되어있는 청크
	stAllocChunk<T>* _afflicatedChunk;

	stAllocTlsNode(){
	}
	stAllocTlsNode(stAllocChunk<T>* afflicatedChunk){
		
		_afflicatedChunk = afflicatedChunk;

	}

};

static constexpr int NODE_NUM = 100;

template <typename T>
struct stAllocChunk{
		
public:

	int _leftFreeCnt;
	int _allocNum;
	//stAllocTlsNode<T>* _allocNode;

	alignas(64) stAllocTlsNode<T>* _nodes;	
	stAllocTlsNode<T>* _nodeEnd;

	int _nodeNum;

	stAllocChunk(){
		
		_nodes = (stAllocTlsNode<T>*)malloc(sizeof(stAllocTlsNode<T>) * NODE_NUM);
		ZeroMemory(_nodes, sizeof(stAllocTlsNode<T>) * NODE_NUM);

		_leftFreeCnt = NODE_NUM;

		_allocNum = 0;

		for(int nodeCnt = 0; nodeCnt < NODE_NUM; ++nodeCnt){
			new (&_nodes[nodeCnt]._data) T();
			_nodes[nodeCnt]._afflicatedChunk = this;
		}

	}

	__declspec(noinline) void chunkReset(){
	
		_allocNum = 0;
		_leftFreeCnt = NODE_NUM;
	}

};

template <typename T>
class CObjectFreeListTLS{

public:

	CObjectFreeListTLS(HANDLE heap, bool runConstructor, bool runDestructor);

	T*	allocObject();
	void freeObject(T* object);

	unsigned int getCapacity();
	unsigned int getUsedCount();
	
	

private:
	
	// 각 스레드에서 활용할 청크를 들고있는 tls의 index
	unsigned __int64 _allocChunkTlsIdx;

	// 모든 스레드가 공통적으로 접근하는 free list 입니다.
	// 이곳에서 T type의 node를 큰 덩어리로 들고옵니다.
	CObjectFreeList<stAllocChunk<T>>* _centerFreeList;

	// heap
	HANDLE _heap;
		
	// T type에 대한 생성자 호출 여부
	bool _runConstructor;

	// T type에 대한 소멸자 호출 여부
	bool _runDestructor;

};

template <typename T>
CObjectFreeListTLS<T>::CObjectFreeListTLS(HANDLE heap, bool runConstructor, bool runDestructor){

	_heap = heap;

	_centerFreeList = (CObjectFreeList<stAllocChunk<T>>*)HeapAlloc(_heap, 0, sizeof(CObjectFreeList<stAllocChunk<T>>));
	new (_centerFreeList) CObjectFreeList<stAllocChunk<T>>(_heap, false, false);

	_allocChunkTlsIdx = TlsAlloc();
	if(_allocChunkTlsIdx == TLS_OUT_OF_INDEXES){
		CDump::crash();
	}

	_runConstructor = runConstructor;
	_runDestructor	= runDestructor;


}

template <typename T>
typename T* CObjectFreeListTLS<T>::allocObject(){

	stAllocChunk<T>* chunk = (stAllocChunk<T>*)TlsGetValue(_allocChunkTlsIdx);
	
	if(chunk == nullptr){
		chunk = _centerFreeList->allocObject();
		chunk->chunkReset();
		TlsSetValue(_allocChunkTlsIdx, chunk);
	}
	
	T* allocData = &chunk->_nodes[chunk->_allocNum++]._data;

	if(chunk->_allocNum == NODE_NUM){
		chunk = _centerFreeList->allocObject();		
		chunk->chunkReset();
		TlsSetValue(_allocChunkTlsIdx, chunk);
	}

	if(_runConstructor == true){
		new (allocData) T();
	}

	return allocData;

}

template <typename T>
void CObjectFreeListTLS<T>::freeObject(T* object){

	if(_runDestructor == true){
		object->~T();
	}

	stAllocChunk<T>* chunk = ((stAllocTlsNode<T>*)((unsigned __int64)object + _dataToNodePtr))->_afflicatedChunk;
	int freeCnt = InterlockedDecrement((LONG*)&chunk->_leftFreeCnt);
	if(freeCnt == 0){
		_centerFreeList->freeObject(chunk);
	}

}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getUsedCount(){
	
	return _centerFreeList->getUsedCount();

}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getCapacity(){
	
	return _centerFreeList->getCapacity();

}

#endif