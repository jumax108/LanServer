#pragma once

using UINT = unsigned int;

template<typename T>
class CQueue {

public:

	CQueue(UINT capacity);
	~CQueue();
	
	inline UINT size() {
		return _size;
	}
	inline UINT capacity() {
		return _capacity;
	}

	bool push(T in);
	bool pop();
	bool front(T* out);

private:

	T* _data;
	UINT _size;
	UINT _capacity;
	UINT _rear;
	UINT _front;

};

template<typename T>
CQueue<T>::CQueue(UINT capacity) {

	_data = new T[capacity];
	_capacity = capacity;
	_size = 0;
	_rear = 0;
	_front = 0;

}

template<typename T>
CQueue<T>::~CQueue() {

	delete[](_data);

}

template<typename T>
bool CQueue<T>::push(T in) {

	if (_size == _capacity) {
		return false;
	}

	_data[_rear] = in;
	_rear = (_rear + 1) % _capacity;
	_size += 1;

	return true;

}

template<typename T>
bool CQueue<T>::pop() {

	if (_size == 0) {
		return false;
	}

	_front = (_front + 1) % _capacity;
	_size -= 1;

	return true;

}

template<typename T>
bool CQueue<T>::front(T* out) {

	if (_size == 0) {
		return false;
	}

	*out = _data[_front];

	return true;

}