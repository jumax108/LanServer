#pragma once

#include <Windows.h>

#include "common.h"

#if defined(STACK_LOG)
	#define push(data) _push(data, __FILEW__, __LINE__)
	#define pop() _pop(__FILEW__, __LINE__)
#else 
	#define push(data) _push(data)
	#define pop() _pop()
#endif

template <typename T>
class CStack {

	struct stNode;

public:

	CStack(unsigned int capacity);
	~CStack();

	inline unsigned int size() {
		return _size;
	}

	inline unsigned int capacity() {
		return _capacity;
	}

	bool _push(T data
		#if defined(STACK_LOG)
			,const wchar_t* file, int line
		#endif
	);
	bool _pop(
		#if defined(STACK_LOG)
			const wchar_t* file, int line
		#endif
	);
	bool front(T* out);

private:

	HANDLE _heap;
	stNode* _data;
	unsigned int _size;
	unsigned int _capacity;

	struct stNode{

	public:
		inline stNode(T& data){
			_data = data;
			#if defined(STACK_LOG)
				_pushFile = nullptr;
				_pushLine = 0;
				_popFile  = nullptr;
				_popFile  = 0;
			#endif
		}
		#if defined(STACK_LOG)
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
		#if defined(STACK_LOG)
			wchar_t* _pushFile;
			int _pushLine;
			wchar_t* _popFile;
			int _popLine;
		#endif
	};

};

template<typename T>
CStack<T>::CStack(unsigned int capacity) {

	_heap = HeapCreate(0, 0, 0);
	_data = (stNode*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(stNode) * capacity);
	_capacity = capacity;
	_size = 0;

}

template<typename T>
CStack<T>::~CStack() {
	HeapFree(_heap, 0, _data);
	HeapDestroy(_heap);
}

template<typename T>
bool CStack<T>::_push(T data
	#if defined(STACK_LOG)
		,const wchar_t* file, int line
	#endif
) {

	if (_size == _capacity) {
		return false;
	}

	stNode* node = &_data[_size];
	new (node) stNode(data);

	#if defined(STACK_LOG)
		node->pushLog(file, line);
	#endif

	_size += 1;

	return true;

}

template<typename T>
bool CStack<T>::_pop(
	#if defined(STACK_LOG)
		const wchar_t* file, int line
	#endif
) {

	if (_size == 0) {
		return false;
	}

	#if defined(STACK_LOG)
		_data[_size - 1].popLog(file, line);
	#endif

	_size -= 1;

	return true;

}

template<typename T>
bool CStack<T>::front(T* out) {

	if (_size == 0) {
		return false;
	}
	
	*out = _data[_size - 1]._data;

	return true;

}