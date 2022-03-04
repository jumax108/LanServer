#ifndef __OBJECT_FREE_LIST__

class CAllocList;

#define __OBJECT_FREE_LIST__

//#define OBJECT_FREE_LIST_SAFE

#ifdef OBJECT_FREE_LIST_SAFE
	#define allocObject() _allocObject(__FILEW__, __LINE__)
	#define freeObject(x) _freeObject(x, __FILEW__, __LINE__)
#endif

//#define toNode(ptr) ((stAllocNode<T>*)((unsigned __int64)ptr & 0x000007FFFFFFFFFF))
//#define toPtr(cnt, pNode) ((void*)((unsigned __int64)pNode | cnt))


#define toNode(ptr) ((stAllocNode<T>*)((unsigned __int64)ptr & 0x000007FFFFFFFFFF))
#define toPtr(cnt, pNode) ((void*)((unsigned __int64)pNode | (cnt << 43)))

// 데이터 포인터(stNode->data)에 이 값을 더하면 노드 포인터(stNode)가 된다 !
static constexpr unsigned __int64 _dataPtrToNodePtr = 0;

template<typename T>
struct stAllocNode {
	stAllocNode() {

		nextPtr = nullptr;
		//used = false;

		#if defined(OBJECT_FREE_LIST_SAFE)
			underFlowCheck = (void*)0xF9F9F9F9F9F9F9F9;
			overFlowCheck = (void*)0xF9F9F9F9F9F9F9F9;
		#endif
	}

	#if defined(OBJECT_FREE_LIST_SAFE)
		// f9로 초기화해서 언더플로우 체크합니다.
		void* underFlowCheck;
	#endif

	// alloc 함수에서 리턴할 실제 데이터
	alignas(64) T data;
	
	#if defined(OBJECT_FREE_LIST_SAFE)
		// f9로 초기화해서 오버플로우 체크합니다.
		void* overFlowCheck;
	#endif

	// 할당할 다음 노드
	void* nextPtr;

	/*
	// 노드가 사용중인지 확인
	bool used;
	*/

	#ifdef OBJECT_FREE_LIST_SAFE
		// 소스 파일 이름
		const wchar_t* allocSourceFileName;
		const wchar_t* freeSourceFileName;

		// 소스 라인
		int allocLine;
		int freeLine;
	#endif
};

template<typename T>
class CObjectFreeList
{
public:

	CObjectFreeList(HANDLE heap, bool runConstructor, bool runDestructor, int _capacity = 0);
	~CObjectFreeList();

	#ifdef OBJECT_FREE_LIST_SAFE
		T* _allocObject(const wchar_t*, int);
		int _freeObject(T* data, const wchar_t*, int);
	#else
		T* allocObject();
		int freeObject(T* data);
	#endif

	inline unsigned int getCapacity() { return _capacity; }
	inline unsigned int getUsedCount() { return _usedCnt; }

private:

	// 메모리 정리용
	// 단순 리스트
	struct stSimpleListNode {
		stAllocNode<T>* ptr;
		stSimpleListNode* next;
	};

	
	// 사용 가능한 노드를 리스트의 형태로 저장합니다.
	// 할당하면 제거합니다.
	void* _freePtr;
	// 리스트 변경 횟수
	// ABA 문제를 회피하기 위해 사용합니다.
	unsigned __int64 _nodeChangeCnt = 0;
	
	// 현재 할당된 노드 개수
	unsigned int _usedCnt;

	// 메모리 할당, 해제를 위한 힙
	alignas(64) HANDLE _heap;

	// 전체 노드 개수
	unsigned int _capacity;
	
	// freeList 소멸자에서 메모리 정리용으로 사용합니다.
	// new한 포인터들
	stSimpleListNode* _totalAllocList;

		
	// 할당 시, 생성자 실행 여부를 나타냅니다.
	bool _runConstructor;

	// 해제 시, 소멸자 실행 여부를 나타냅니다.
	bool _runDestructor;
	
	

};

template <typename T>
CObjectFreeList<T>::CObjectFreeList(HANDLE heap, bool runConstructor, bool runDestructor, int size) {

	_totalAllocList = nullptr;
	_freePtr = nullptr;

	_capacity = size;
	_usedCnt = 0;

	_heap = heap;
	_runConstructor = runConstructor;
	_runDestructor = runDestructor;
	
	// 실제 노드와 노드의 data와의 거리 계산
	/*
	stAllocNode<T> tempNode;
	if(_dataPtrToNodePtr != (unsigned __int64)&tempNode - (unsigned __int64)&tempNode.data){
		CDump::crash();
	}
	*/

	if (size == 0) {
		return;
	}

	for(int nodeCnt = 0; nodeCnt < size; ++nodeCnt){

		// 미리 만들어놓을 개수만큼 노드를 만들어 놓음

		stAllocNode<T>* newNode = (stAllocNode<T>*)HeapAlloc(_heap, 0, sizeof(stAllocNode<T>));
		new (newNode) stAllocNode<T>;
		newNode->nextPtr = _freePtr;
		_freePtr = newNode;

		{
			// 전체 alloc list에 추가
			// 소멸자에서 일괄적으로 메모리 해제하기 위함
			stSimpleListNode* totalAllocNode = (stSimpleListNode*)HeapAlloc(_heap, 0, sizeof(stSimpleListNode));

			totalAllocNode->ptr = newNode;
			totalAllocNode->next = _totalAllocList;

			_totalAllocList = totalAllocNode;

		}

	}


}

template <typename T>
CObjectFreeList<T>::~CObjectFreeList() {

	while(_totalAllocList != nullptr){
		stAllocNode<T>* freeNode = _totalAllocList->ptr;
		HeapFree(_heap, 0, freeNode);
		_totalAllocList = _totalAllocList->next;
	}
	
}

template<typename T>
#ifdef OBJECT_FREE_LIST_SAFE
T* CObjectFreeList<T>::_allocObject(const wchar_t* fileName, int line) {
#else
T* CObjectFreeList<T>::allocObject(){
#endif
	stAllocNode<T>* allocNode = nullptr;
	stAllocNode<T>* nextNode = nullptr;
	stAllocNode<T>* freeNode = nullptr;

	void* freePtr = nullptr;
	void* nextPtr = nullptr;
	
	unsigned __int64 nodeChangeCnt = 0;

	InterlockedIncrement64((LONG64*)&_nodeChangeCnt);
	do {
		// 원본 데이터 복사
		freePtr = _freePtr;
		nodeChangeCnt = _nodeChangeCnt;

		freeNode = toNode(freePtr);

		allocNode = freeNode;

		if (allocNode == nullptr) {

			// 추가 할당
			allocNode = (stAllocNode<T>*)HeapAlloc(_heap, 0, sizeof(stAllocNode<T>));
			//allocNode->nextPtr = nullptr;
			new (allocNode) stAllocNode<T>;
		
			// 전체 alloc list에 추가
			// 소멸자에서 일괄적으로 메모리 해제하기 위함	
		
			stSimpleListNode* totalAllocNode = (stSimpleListNode*)HeapAlloc(_heap, 0, sizeof(stSimpleListNode));
			stSimpleListNode* totalAllocList = nullptr;

			do {

				totalAllocList = _totalAllocList;

				totalAllocNode->ptr = allocNode;
				totalAllocNode->next = totalAllocList;

				_totalAllocList = totalAllocNode;

			} while( InterlockedCompareExchange64((LONG64*)&_totalAllocList, (LONG64)totalAllocNode, (LONG64)totalAllocList) != (LONG64)totalAllocList );

			InterlockedIncrement(&_capacity);

			break;

		}
		nextPtr = allocNode->nextPtr;
		
	} while(InterlockedCompareExchange64((LONG64*)&_freePtr, (LONG64)nextPtr, (LONG64)freePtr) != (LONG64)freePtr);

	InterlockedIncrement(&_usedCnt);

	T* data = &allocNode->data;
	// 생성자 실행
	if(_runConstructor == true){
		new (data) T();
	}

	// 노드를 사용중으로 체크함
	//allocNode->used = true;

	// 할당 요청한 소스파일과 소스라인을 기록함
	#ifdef OBJECT_FREE_LIST_SAFE
		allocNode->allocSourceFileName = fileName;
		allocNode->allocLine = line;
	#endif
	
	
	return data;
}

template <typename T>
#ifdef OBJECT_FREE_LIST_SAFE
int CObjectFreeList<T>::_freeObject(T* data, const wchar_t* fileName, int line) {
#else
int CObjectFreeList<T>::freeObject(T* data){
#endif
	/*
		log code
		16^1의 자리
			2 : free
		16^0의 자리
			1 : 함수 진입
			2 : 함수 리턴
	*/
	void* freePtr;

	stAllocNode<T>* usedNode = (stAllocNode<T>*)(((char*)data) + _dataPtrToNodePtr);
	#ifdef OBJECT_FREE_LIST_SAFE
		usedNode->freeSourceFileName = fileName;
		usedNode->freeLine = line;
	#endif
	
	unsigned __int64 nodeChangeCnt = 0;
	InterlockedIncrement64((LONG64*)&_nodeChangeCnt);

	#if defined(OBJECT_FREE_LIST_SAFE)
		// 중복 free 체크
		if(usedNode->used == false){
			CDump::crash();
		}

		// 오버플로우 체크
		if((unsigned __int64)usedNode->overFlowCheck != 0xF9F9F9F9F9F9F9F9){
			CDump::crash();
		}

		// 언더플로우 체크
		if((unsigned __int64)usedNode->underFlowCheck != 0xF9F9F9F9F9F9F9F9){
			CDump::crash();
		}
	#endif

	// 노드의 사용중 플래그를 내림
	//usedNode->used = false;

	// 소멸자 실행
	if(_runDestructor == true){
		data->~T();
	}

	stAllocNode<T>* freeNode;
	stAllocNode<T>* nextNode;

	void* nextPtr;

	do {
	
		// 원본 데이터 복사
		freePtr = _freePtr;
		nodeChangeCnt = _nodeChangeCnt;

		freeNode = toNode(freePtr);
	
		// free node로 등록함
		usedNode->nextPtr = freePtr;

		nextNode = usedNode;
		nextPtr = toPtr(nodeChangeCnt, nextNode);

	} while(InterlockedCompareExchange64((LONG64*)&_freePtr, (LONG64)nextPtr, (LONG64)freePtr) != (LONG64)freePtr);
	
	InterlockedDecrement(&_usedCnt);

	return 0;

}

#endif