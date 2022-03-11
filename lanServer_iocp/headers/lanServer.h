#pragma once

#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
#include <WS2tcpip.h>

#include <thread>
#include <new>
#include <windows.h>

///////////////////////////////////////////////////////////////////
// lib
#include "dump/headers/dump.h"
#include "log/headers/log.h"
#include "protocolBuffer/headers/protocolBuffer.h"
#include "packetPointer/headers/packetPointer.h"
#include "stack/headers/stack.h"
#include "queue/headers/queue.h"
#include "ringBuffer/headers/ringBuffer.h"

#pragma comment(lib, "lib/dump/dump")
#pragma comment(lib, "lib/log/log")
#pragma comment(lib, "lib/protocolBuffer/protocolBuffer")
#pragma comment(lib, "lib/pakcetPointer/packetPointer")
#pragma comment(lib, "lib/ringBuffer/ringBuffer")
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// header
#include "common.h"
#include "packetPointer_LanServer.h"
///////////////////////////////////////////////////////////////////


class CLanServer{

	struct stSession;

public:

	CLanServer();

	// �ʿ��� �޸𸮸� �Ҵ��ϰ� ������ �����մϴ�.
	void start(const wchar_t* serverIP, unsigned short serverPort,
			int createWorkerThreadNum, int runningWorkerThreadNum,
			unsigned short maxSessionNum, bool onNagle,
			unsigned int sendBufferSize, unsigned int recvBufferSize,
			unsigned int sendBufferMaxCapacity, unsigned int recvBufferMaxCapacity); 
	// ��� �޸𸮸� �����ϰ� ������ �����մϴ�.
	void stop(); 
	
	// ���� �������� ���� ���� ��ȯ�մϴ�.
	inline unsigned __int64 getSessionCount();
	
	// sessionID �� �ش��ϴ� ���� �����մϴ�.
	bool disconnect(unsigned __int64 sessionID); 
	// sessinoID �� �ش��ϴ� ���ǿ� ������ �����մϴ�.
	bool sendPacket(unsigned __int64 sessionID, CPacketPtr_Lan packet);
	 
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
	virtual void onRecv(unsigned __int64 sessionID, CPacketPointer pakcet) = 0;
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
	virtual void onSend(unsigned __int64 sessionID, int sendSize) = 0;

	// ���� ��Ȳ���� ȣ��˴ϴ�.
	virtual void onError(int errorCode, const wchar_t* errorMsg) = 0;


private:

	static HANDLE _heap;

	stSession* _sessionArr;

	unsigned __int64 _sessionNum;
	unsigned __int64 _sessionCnt;
	CStack<int>* _sessionIndexStack;

	HANDLE _acceptThread;

	int _workerThreadNum;
	HANDLE* _workerThread;

	SOCKET _listenSocket;

	HANDLE _iocp;


	// logger
	CLog log;

	// thread ������ event
	HANDLE _stopEvent;

	void sendPost(stSession* session);
	void recvPost(stSession* session);

	static unsigned __stdcall completionStatusFunc(void* args);
	static unsigned __stdcall acceptFunc(void* args);

	void checkCompletePacket(stSession* session, CRingBuffer* recvBuffer);

	void release(unsigned __int64 sessionID);

	struct stSession{
		
		stSession(unsigned int sendQueueSize, unsigned int recvBufferSize);
		~stSession();

		// ID�� ���� 6����Ʈ�� ���� �޸𸮿� ���� ���� Ƚ��
		// ���� 2����Ʈ�� ���� �ε���
		unsigned __int64 _sessionID; // ���� ���� �߿��� ������ ���� ID
	
		SOCKET _sock;
	
		unsigned int _ip;
		unsigned short _port;

		CQueue<CPacketPointer> _sendQueue;
		CRingBuffer _recvBuffer;
		
		// send�� 1ȸ�� �����ϱ� ���� �÷���
		bool _isSent;
		
		// �� 32��Ʈ�� ������ �÷��� ��ȭ�� ioCnt�� 0���� ���ÿ� üũ�ϱ� ����
		alignas(16) bool _beRelease;
		// recv, send io�� ��û�Ǿ� �ִ� Ƚ���Դϴ�.
		// recv�� �׻� ��û�Ǿ��ֱ� ������ ioCnt�� �ּ� 1�Դϴ�.
		// ioCnt�� 0�̵Ǹ� ������ ���� �����Դϴ�.
		public: unsigned char _ioCnt; 

		// recv �Լ� �ߺ� ȣ�� ������
		bool _recvPosted;
		
		OVERLAPPED _sendOverlapped;
		OVERLAPPED _recvOverlapped;

		CPacketPointer* _packets;
		int _packetCnt;

		CRITICAL_SECTION _lock;
	};
};

CLanServer::stSession::stSession(unsigned int sendQueueSize, unsigned int recvBufferSize):
	_recvBuffer(recvBufferSize), _sendQueue(sendQueueSize)
{
	_sessionID = 0;
	_sock = NULL;
	_ip = 0;
	_port = 0;
	_isSent = false;
	_beRelease = false;
	_ioCnt = 0;

	ZeroMemory(&_sendOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&_recvOverlapped, sizeof(OVERLAPPED));
	_packets = (CPacketPointer*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(CPacketPointer) * MAX_PACKET);
	_packetCnt = 0;

	_recvPosted = false;

	InitializeCriticalSection(&_lock);
}

CLanServer::stSession::~stSession(){
	HeapFree(_heap, 0, _packets);
	DeleteCriticalSection(&_lock);
}