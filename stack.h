#pragma once

template <typename T>
class CStack {

public:

	CStack(UINT capacity);
	~CStack();

	inline void size(UINT* out) {
		*out = _size;
	}

	inline void capacity(UINT* out) {
		*out = _capacity;
	}

	bool pushData(T _data);
	bool popData();
	bool frontData(T* out);

private:

	T* _data;
	UINT _size;
	UINT _capacity;

};

template<typename T>
CStack<T>::CStack(UINT capacity) {

	_data = new T[capacity];
	_capacity = capacity;
	_size = 0;

}

template<typename T>
CStack<T>::~CStack() {
	delete[] _data;
}

template<typename T>
bool CStack<T>::pushData(T data) {

	if (_size == _capacity) {
		return false;
	}

	_data[_size] = data;
	_size += 1;

	return true;

}

template<typename T>
bool CStack<T>::popData() {

	if (_size == 0) {
		return false;
	}

	_size -= 1;

	return true;

}

template<typename T>
bool CStack<T>::frontData(T* out) {

	if (_size == 0) {
		return false;
	}
	
	*out = _data[_size - 1];

	return true;

}