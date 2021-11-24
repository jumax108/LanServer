#pragma once

#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
#include <WS2tcpip.h>
#include <stdexcept>
#include <thread>
#include <new>

#include "ObjectFreeList.h"
#include "serverError.h"
#include "stack.h"
#include "stringParser.h"
#include "ringBuffer.h"
#include "protocolBuffer.h"
#include "common.h"

class CLanServer{

public:

	CLanServer();

	// �ʿ��� �޸𸮸� �Ҵ��ϰ� ������ �����մϴ�.
	void start(const wchar_t* configFileName); 
	// ��� �޸𸮸� �����ϰ� ������ �����մϴ�.
	void stop(); 
	
	// ���� �������� ���� ���� ��ȯ�մϴ�.
	unsigned __int64 getSessionCount();
	
	// sessionID �� �ش��ϴ� ���� �����մϴ�.
	bool disconnect(unsigned __int64 sessionID); 
	// sessinoID �� �ش��ϴ� ���ǿ� ������ �����մϴ�.
	bool sendPacket(unsigned __int64 sessionID, CProtocolBuffer* packet);
	 
	// Ŭ���̾�Ʈ�� ������ �õ��� ���¿��� ȣ��˴ϴ�.
	// ��ȯ�� ���� ���� ������ ����մϴ�.
	// return true = ���� ��, ���� �ʱ�ȭ
	// return false = ������ ����
	virtual bool onConnectRequest(unsigned int ip, unsigned short port) = 0;
	// Ŭ���̾�Ʈ�� ������ �Ϸ��� ���¿��� ȣ��˴ϴ�.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) = 0;
	// Ŭ���̾�Ʈ�� ������ �����Ǹ� ȣ��˴ϴ�.
	virtual void onClientLeave(unsigned __int64 sessionID) = 0;

	// Ŭ���̾�Ʈ���� �����͸� �����ϸ� ȣ��˴ϴ�.
	virtual void onRecv(unsigned __int64 sessionID, CProtocolBuffer* pakcet) = 0;
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
	virtual void onSend(unsigned __int64 sessionID, int sendSize) = 0;

	// ���� ��Ȳ���� ȣ��˴ϴ�.
	virtual void onError(SERVER_ERROR errorCode, const wchar_t* errorMsg) = 0;

	// ���� ���� �� ������ �߻��ϸ� throw �˴ϴ�.
	struct stStartError{
		int _errorCode;
		const wchar_t* _errorMessage;
		stStartError(int errorCode, const wchar_t* errorMessage){
			_errorCode = errorCode;
			_errorMessage = errorMessage;
		}
	};


private:

	struct stSession{
		
		stSession(unsigned int sendBufferSize = 1, unsigned int recvBufferSize = 1):
			_sendBuffer(sendBufferSize), _recvBuffer(recvBufferSize){
			_sessionID = (unsigned __int64)this;
			_sock = NULL;
			_ip = 0;
			_port = 0;
			_isSent = false;
			_ioCnt = 0;
			ZeroMemory(&_sendOverlapped, sizeof(OVERLAPPED));
			ZeroMemory(&_recvOverlapped, sizeof(OVERLAPPED));
			InitializeSRWLock(&_lock);
			_beDisconnect = false;
		}

		void lock(){
			AcquireSRWLockExclusive(&_lock);
		}

		void unlock(){
			ReleaseSRWLockExclusive(&_lock);
		}

		// ID�� ���� 21��Ʈ�� ���� �޸𸮿� ���� ���� Ƚ��
		// ���� 43��Ʈ�� ���� �޸𸮿� ���� �ּ�
		unsigned __int64 _sessionID; // ���� ���� �߿��� ������ ���� ID
	
		SOCKET _sock;
	
		unsigned int _ip;
		unsigned short _port;

		CRingBuffer _sendBuffer;
		CRingBuffer _recvBuffer;

		bool _isSent; // send�� 1ȸ�� �����ϱ� ���� �÷���

		// recv, send io�� ��û�Ǿ� �ִ� Ƚ���Դϴ�.
		// recv�� �׻� ��û�Ǿ��ֱ� ������ ioCnt�� �ּ� 1�Դϴ�.
		// ioCnt�� 0�̵Ǹ� ������ ���� �����Դϴ�.
		short _ioCnt; 

		OVERLAPPED _sendOverlapped;
		OVERLAPPED _recvOverlapped;

		SRWLOCK _lock;

		bool _beDisconnect;
	};

	CObjectFreeList<stSession>* _sessionFreeList;
	SRWLOCK _sessionFreeListLock;

	void sessionFreeListLock();
	void sessionFreeListUnlock();

	unsigned __int64 _sessionNum;
	unsigned __int64 _sessionCnt;

	unsigned __int64 _sessionPtrMask;
	unsigned __int64 _sessionIDMask;

	int _sendBufferSize;
	int _recvBufferSize;

	HANDLE _acceptThread;

	HANDLE* _workerThread;

	SOCKET _listenSocket;

	HANDLE _iocp;

	void sendPost(stSession* session);
	void recvPost(stSession* session);

	static unsigned __stdcall completionStatusFunc(void* args);
	static unsigned __stdcall acceptFunc(void* args);

	void checkCompletePacket(unsigned __int64 sessionID, CRingBuffer* recvBuffer);

};