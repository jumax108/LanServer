#pragma once

template <typename T>
class CLockFreeStack{

public:

	CLockFreeStack();
	~CLockFreeStack();

	bool push(T data, HANDLE thread = NULL);
	bool pop(T* data, HANDLE thread = NULL);

	unsigned int getSize();

	unsigned int getNodeCap();

private:

	struct stNode{
		
		stNode(){
			_data = NULL;
			_next = nullptr;
		}

		T _data;
		void* _next;
	};

	void* _top;
	// 재사용 메모리를 검사하기 위한 변수
	// push하면 계속 증가시키고 21개 비트를 ptr 값에 넣어서 비교함
	unsigned __int64 _nodeChangeCnt;
	unsigned int _size;

	// 재사용 횟수용 마스크
	alignas(64) unsigned __int64 _reUseCntMask; 
	// stNode 포인터용 마스크
	unsigned __int64 _pointerMask;  
	

	unsigned short _capacity;
	CObjectFreeList<stNode>* _nodeFreeList;


	HANDLE _heap;


};
 
template<typename T>
CLockFreeStack<T>::CLockFreeStack(){

	_size = 0;
	_top = 0;

	//_pushCnt = 0;

	_reUseCntMask = 0xFFFFF80000000000;
	_pointerMask  = 0x000007FFFFFFFFFF;

	_heap = HeapCreate(0, 0, 0);
	_nodeFreeList = new CObjectFreeList<stNode>(_heap, false, false);

}

template<typename T>
CLockFreeStack<T>::~CLockFreeStack(){
	delete _nodeFreeList;
}

template <typename T>
bool CLockFreeStack<T>::push(T data, HANDLE thread){
	
	//unsigned __int64 pushCnt = InterlockedIncrement(&_pushCnt);
	stNode* newNode = _nodeFreeList->allocObject();
	newNode->_data = data;
	
	void* newNodePtr = nullptr;
	void* top = nullptr;
	stNode* topNode = nullptr;

	do {
		
		top = _top;

		topNode = (stNode*)((unsigned __int64)top & _pointerMask);
		newNode->_next = top;
		newNodePtr = (void*)((_nodeChangeCnt << 43) | (unsigned __int64)newNode);
		
	} while(InterlockedCompareExchange64((__int64*)&_top, (__int64)newNodePtr, (__int64)top) != (__int64)top);
	
	InterlockedIncrement64((LONG64*)&_nodeChangeCnt);

	return true;

}

template<typename T>
bool CLockFreeStack<T>::pop(T* data, HANDLE thread){

	//unsigned __int64 popCnt = InterlockedIncrement64((LONG64*)&_popCnt);

	stNode* topNode			= nullptr;
	stNode* topNextNode		= nullptr;
	void* topNextPtr		= nullptr;
	void* top				= nullptr;

	do {

		top = _top;
		//popCnt = _popCnt;

		topNode = (stNode*)((unsigned __int64)top & _pointerMask);
		topNextPtr = topNode->_next;
		topNextNode = (stNode*)((unsigned __int64)topNextPtr & _pointerMask);

	} while(InterlockedCompareExchange64((__int64*)&_top, (__int64)topNextPtr, (__int64)top) != (__int64)top);
	
	InterlockedIncrement64((LONG64*)&_nodeChangeCnt);

	*data = topNode->_data;

	_nodeFreeList->freeObject(topNode);

	return true;
}

template <typename T>
unsigned int CLockFreeStack<T>::getSize(){
	return _size;
}

template <typename T>
unsigned int CLockFreeStack<T>::getNodeCap(){
	return _nodeFreeList->getCapacity();
}