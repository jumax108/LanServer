#pragma once

#include <Windows.h>

#include "common.h"

#if defined(QUEUE_LOG)
	#define push(data) _push(data, __FILEW__, __LINE__)
	#define pop() _pop(__FILEW__, __LINE__)
#else 
	#define push(data) _push(data)
	#define pop() _pop()
#endif

template<typename T>
class CQueue {

	struct stNode;

public:

	CQueue(unsigned int capacity, unsigned int maxCapacity = 0xFFFFFFFF);
	~CQueue();
	
	inline unsigned int size() {
		return _size;
	}
	inline unsigned int capacity() {
		return _capacity;
	}

	bool _push(T in
		#if defined(QUEUE_LOG)
			,const wchar_t* file, int line
		#endif
	);
	bool _pop(
		#if defined(QUEUE_LOG)
			const wchar_t* file, int line
		#endif
	);
	bool front(T* out);

private:

	stNode* _data;
	unsigned int _size;
	unsigned int _capacity;
	unsigned int _rear;
	unsigned int _front;
	unsigned int _maxCapacity;

	void resize(unsigned int capapcity);

	HANDLE _heap;

	struct stNode{
		friend CQueue;

	public:
		inline stNode(T& data){
			_data = data;
			#if defined(QUEUE_LOG)
				_pushFile = nullptr;
				_pushLine = 0;
				_popFile  = nullptr;
				_popFile  = 0;
			#endif
		}
		#if defined(QUEUE_LOG)
			inline void pushLog(const wchar_t* file, int line){
				_pushFile = (wchar_t*)file;
				_pushLine = line;
			}
			inline void popLog(const wchar_t* file, int line){
				_popFile = (wchar_t*)file;
				_popLine = line;
			}
		#endif
	private:
		T _data;
		#if defined(QUEUE_LOG)
			wchar_t* _pushFile;
			int _pushLine;
			wchar_t* _popFile;
			int _popLine;
		#endif
	};
};

template<typename T>
CQueue<T>::CQueue(unsigned int capacity, unsigned int maxCapacity) {

	_heap = HeapCreate(0,0,0);
	_data = (stNode*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(stNode) * capacity);

	_capacity = capacity;
	_maxCapacity = maxCapacity;
	_size = 0;
	_rear = 0;
	_front = 0;

}

template<typename T>
CQueue<T>::~CQueue() {

	HeapFree(_heap, 0, _data);
	HeapDestroy(_heap);

}

template <typename T>
void CQueue<T>::resize(unsigned int capacity){

	stNode* oldData = _data;
	_data = (stNode*)HeapAlloc(_heap, 0, sizeof(stNode) * capacity);

	memcpy(_data, oldData, sizeof(stNode) * _capacity);
	
	_capacity = capacity;
	HeapFree(_heap, 0, oldData);

}

template<typename T>
bool CQueue<T>::_push(T in
	#if defined(QUEUE_LOG)
		,const wchar_t* file, int line
	#endif
) {

	if (_size == _capacity) {
		if(_capacity == _maxCapacity){
			return false;
		}
		
		unsigned __int64 nextCapacity = (unsigned __int64)_capacity * 2 + 1;
		if(nextCapacity >= (unsigned __int64)_maxCapacity){
			nextCapacity = (unsigned __int64)_maxCapacity;
		}
		resize(nextCapacity);
	}

	stNode* node = &_data[_rear];
	new (node) stNode(in);

	#if defined(QUEUE_LOG)
		node->pushLog(file, line);
	#endif

	_rear = (_rear + 1) % _capacity;
	_size += 1;

	return true;

}

template<typename T>
bool CQueue<T>::_pop(
	#if defined(QUEUE_LOG)
		const wchar_t* file, int line
	#endif
) {

	if (_size == 0) {
		return false;
	}

	#if defined(QUEUE_LOG)
		_data[_front].popLog(file, line);
	#endif

	_data[_front]._data.~T();

	_front = (_front + 1) % _capacity;
	_size -= 1;

	return true;

}

template<typename T>
bool CQueue<T>::front(T* out) {

	if (_size == 0) {
		return false;
	}

	*out = _data[_front]._data;

	return true;

}