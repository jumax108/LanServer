#pragma once

extern CDump dump;

template <typename T>
class CLockFreeQueue{

public:

	CLockFreeQueue();

	bool push(T data);
	bool pop(T* data);

	unsigned __int64 getSize();
	unsigned __int64 getCapacity();

private:

	struct stNode{
		stNode(){
			_next = nullptr;
			_data = (T*)malloc(sizeof(T));
		}
		void* _next;
		T* _data;
	};
	
	struct stLogLine{

		char _code;
		HANDLE _thread;
		unsigned __int64 _logCnt;

		// 몇번째에 진입한 함수인지를 나타냅니다.
		unsigned __int64 _functionCnt;
		
		void*	_headPtr;
		stNode* _headNode;
		stNode* _nextHeadNode;
		stNode* _popNode;
		void*	_popData;

		void*	_tailPtr;
		stNode* _tailNode;
		stNode* _tailNodeNext;
		stNode* _nextTailNode;
		stNode* _pushNode;
		void*   _pushPtr;
		void*	_pushData;
	};
	
	void* _head;
	alignas(64) void* _tail;

	unsigned __int64 _size = 0;

	// 몇번째의 함수 호출인지 확인, push pop 진입 시에 증가하고 진입합니다.
	// 같은 값이면 동일한 스레드, 동일한 함수가 끝나지 않았음을 보장합니다.
	unsigned __int64 _functionCnt = 0;
	unsigned __int64 _useCnt = 0;

	alignas(64) unsigned __int64 _useCntMask	= 0xFFFFF80000000000;
	unsigned __int64 _pointerMask	= 0x000007FFFFFFFFFF;

	HANDLE _heap;
	CObjectFreeList<stNode>* _nodeFreeList;
};

template <typename T>
CLockFreeQueue<T>::CLockFreeQueue(){


	_heap = HeapCreate(0, 0, 0);
	_nodeFreeList = new CObjectFreeList<stNode>(_heap, false, false);

	stNode* node = _nodeFreeList->allocObject();
	_head = node;
	_tail = node;

}

template <typename T>
bool CLockFreeQueue<T>::push(T data){
	
	unsigned __int64 functionCnt = InterlockedIncrement64((LONG64*)&_functionCnt);
	
	stNode* newNode = _nodeFreeList->allocObject();

	*newNode->_data = data;
	newNode->_next = nullptr;

	void* newPtr;
	void* tail;
	void* tailNextPtr = nullptr;
	void* head;

	stNode* tailNode;
	stNode* headNode;
	stNode* tailNextNode;
	
	unsigned __int64 loopCnt = 0;
	
	unsigned __int64 useCnt;
	
	_useCnt += 1;
	do{
		
		// tail이 null이 아닐 때까지 변경
		do{

			tail = _tail;
			tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
			tailNextPtr = tailNode->_next;

			if(tailNextPtr == nullptr){
				break;
			}
						

		}while(InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)tailNextPtr, (LONG64)tail) != (LONG64)tail);


		tail = _tail;
		//head = _head;
		useCnt = _useCnt;

		tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
		//headNode = (stNode*)((unsigned __int64)head & _pointerMask);
		//tailNextPtr = tailNode->_next;
		//tailNextNode = (stNode*)((unsigned __int64)tailNextPtr & _pointerMask);
		
		newPtr = (void*)((useCnt << 43) | (unsigned __int64)newNode);

		//loopCnt+=1;
			
	} while( InterlockedCompareExchange64((LONG64*)&tailNode->_next, (LONG64)newPtr, (LONG64)nullptr ) != (LONG64)nullptr );
				
	//tailNextPtr = tailNode->_next;
	//tailNextNode = (stNode*)((unsigned __int64)tailNextPtr & _pointerMask);

	bool result = InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)newPtr, (LONG64)tail) != (LONG64)tail;
	
	InterlockedIncrement64((LONGLONG*)&_size);
	
	return true;

}

template <typename T>
bool CLockFreeQueue<T>::pop(T* data){

	
	unsigned __int64 size = InterlockedDecrement64((LONG64*)&_size);
	if(size < 0){
		InterlockedIncrement64((LONG64*)&_size);
		return false;
	}

	unsigned __int64 functionCnt = InterlockedIncrement64((LONG64*)&_functionCnt);
	
	void* popPtr;
	void* tail;
	void* tailNextPtr = nullptr;
	void* head;
	void* nextHeadPtr;

	stNode* popNode;
	stNode* headNode;
	stNode* eraseNode;
	stNode* tailNode;
	stNode* tailNextNode = nullptr;

	T* popNodeData;

	unsigned __int64 loopCnt = 0;
	
	//InterlockedIncrement64((LONG64*)&_useCnt);
	_useCnt += 1;
	do{
		do { 
			head = _head;
			//tail = _tail;

			headNode = (stNode*)((unsigned __int64)head & _pointerMask);
			popPtr = headNode->_next;
			popNode = (stNode*)((unsigned __int64)popPtr & _pointerMask);
			//tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
			
			do{
				tail = _tail;
				tailNode = (stNode*)((unsigned __int64)tail & _pointerMask);
				tailNextPtr = tailNode->_next;

				if(tailNextPtr == nullptr){
					break;
				}

			}while(InterlockedCompareExchange64((LONG64*)&_tail, (LONG64)tailNextPtr, (LONG64)tail) != (LONG64)tail);

			unsigned __int64 useCnt = _useCnt;

			eraseNode = headNode;

			nextHeadPtr = (void*)((unsigned __int64)popNode | (useCnt << 43));

		} while(popNode == nullptr);
		
		popNodeData = popNode->_data;

		//loopCnt += 1;

	}while(  InterlockedCompareExchange64((LONG64*)&_head, (LONG64)nextHeadPtr, (LONG64)head) != (LONG64)head);
	
	*data = *popNodeData;
	
	_nodeFreeList->freeObject(eraseNode);

	return true;
}

template <typename T>
unsigned __int64 CLockFreeQueue<T>::getSize(){
	return _size;
}


template <typename T>
unsigned __int64 CLockFreeQueue<T>::getCapacity(){
	return _nodeFreeList->getCapacity();
}