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

	// 필요한 메모리를 할당하고 서버를 시작합니다.
	void start(const wchar_t* configFileName); 
	// 모든 메모리를 정리하고 서버를 종료합니다.
	void stop(); 
	
	// 현재 접속중인 세션 수를 반환합니다.
	unsigned __int64 getSessionCount();
	
	// sessionID 에 해당하는 연결 해제합니다.
	bool disconnect(unsigned __int64 sessionID); 
	// sessinoID 에 해당하는 세션에 데이터 전송합니다.
	bool sendPacket(unsigned __int64 sessionID, CProtocolBuffer* packet);
	 
	// 클라이언트가 접속을 시도한 상태에서 호출됩니다.
	// 반환된 값에 따라 연결을 허용합니다.
	// return true = 연결 후, 세션 초기화
	// return false = 연결을 끊음
	virtual bool onConnectRequest(unsigned int ip, unsigned short port) = 0;
	// 클라이언트가 접속을 완료한 상태에서 호출됩니다.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID) = 0;
	// 클라이언트의 연결이 해제되면 호출됩니다.
	virtual void onClientLeave(unsigned __int64 sessionID) = 0;

	// 클라이언트에게 데이터를 전송하면 호출됩니다.
	virtual void onRecv(unsigned __int64 sessionID, CProtocolBuffer* pakcet) = 0;
	// 클라이언트에게서 데이터를 전달받으면 호출됩니다.
	virtual void onSend(unsigned __int64 sessionID, int sendSize) = 0;

	// 에러 상황에서 호출됩니다.
	virtual void onError(SERVER_ERROR errorCode, const wchar_t* errorMsg) = 0;

	// 서버 시작 중 에러가 발생하면 throw 됩니다.
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

		// ID의 상위 21비트는 세션 메모리에 대한 재사용 횟수
		// 하위 43비트는 세션 메모리에 대한 주소
		unsigned __int64 _sessionID; // 서버 가동 중에는 고유한 세션 ID
	
		SOCKET _sock;
	
		unsigned int _ip;
		unsigned short _port;

		CRingBuffer _sendBuffer;
		CRingBuffer _recvBuffer;

		bool _isSent; // send를 1회로 제한하기 위한 플래그

		// recv, send io가 요청되어 있는 횟수입니다.
		// recv는 항상 요청되어있기 때문에 ioCnt는 최소 1입니다.
		// ioCnt가 0이되면 연결이 끊긴 상태입니다.
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