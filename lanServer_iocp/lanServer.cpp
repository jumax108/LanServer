

#include "headers/lanServer.h"

HANDLE CLanServer::_heap = NULL;

CLanServer::CLanServer(){

	_heap = HeapCreate(0, 0, 0);

	_sessionArr = nullptr;
	_sessionNum = 0;
	_sessionCnt = 0;
	_sessionIndexStack = (CStack<int>*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(CStack<int>));

	_acceptThread = NULL;
	_workerThreadNum = 0;
	_workerThread = nullptr;

	_listenSocket = NULL;
	_iocp = NULL;

	_sendCnt = 0;
	_recvCnt = 0;
	
	_sendTPS = 0;
	_recvTPS = 0;
	
	_log.setDirectory(L"log");
	_log.setPrintGroup(LOG_GROUP::LOG_ERROR | LOG_GROUP::LOG_SYSTEM);

	_stopEvent = CreateEvent(nullptr, true, false, L"stopEvent");
}

void CLanServer::disconnect(unsigned __int64 sessionID){
	
	unsigned short sessionIndex = sessionID & SESSION_INDEX_MASK;
	stSession* session = &_sessionArr[sessionIndex];
	
	EnterCriticalSection(&session->_lock);  {
	
		// send buffer �ȿ� �ִ� ��Ŷ�� ����		
		CQueue<CPacketPointer>* sendBuffer = &session->_sendQueue;
		while(sendBuffer->size() > 0){
			CPacketPointer packetPtr;
			sendBuffer->front(&packetPtr);
			sendBuffer->pop();
			packetPtr.decRef();
		}

		closesocket(session->_sock);

		_sessionIndexStack->push(sessionIndex);

		_sessionCnt -= 1;

	} LeaveCriticalSection(&session->_lock);
	

}
bool CLanServer::sendPacket(unsigned __int64 sessionID, CPacketPtr_Lan packet){
	
	unsigned short sessionIndex = sessionID & SESSION_INDEX_MASK;
	stSession* session = &_sessionArr[sessionIndex];

	EnterCriticalSection(&session->_lock); {
		
		CQueue<CPacketPointer>* sendQueue = &session->_sendQueue;
		packet.incRef();
		packet.setHeader();
		sendQueue->push(packet);

		if(session->_isSent == false){
			sendPost(session);
		}

	} LeaveCriticalSection(&session->_lock);

	return true;

}

unsigned CLanServer::completionStatusFunc(void *args){
	
	CLanServer* server = (CLanServer*)args;

	HANDLE iocp = server->_iocp;

	for(;;){
		
		unsigned int transferred;
		OVERLAPPED* overlapped;
		unsigned __int64 sessionID;
		GetQueuedCompletionStatus(iocp, (LPDWORD)&transferred, (PULONG_PTR)&sessionID, &overlapped, INFINITE);
		
		if(overlapped == nullptr){
			break;			
		}
		
		unsigned short sessionIndex = (unsigned short)(sessionID & SESSION_INDEX_MASK);
		stSession* session = &server->_sessionArr[sessionIndex];

		EnterCriticalSection(&session->_lock); { 
		do {

			if(sessionID != session->_sessionID || session->_beRelease == true){
				break;
			}

			SOCKET sock = session->_sock;

			// send �Ϸ� ó��
			if(&session->_sendOverlapped == overlapped){
		
				int packetTotalSize = 0;

				int packetNum = session->_packetCnt;
				CPacketPointer* packets = session->_packets;
				CPacketPointer* packetIter = packets;
				CPacketPointer* packetEnd = packets + packetNum;
				for(; packetIter != packetEnd; ++packetIter){
					packetTotalSize += packetIter->getPacketPoolUsage();
					packetIter->decRef();
				}
				session->_packetCnt = 0;
			
				InterlockedAdd((LONG*)&server->_sendCnt, packetNum);

				server->onSend(sessionID, packetTotalSize);

				CQueue<CPacketPointer>* sendQueue = &session->_sendQueue;
				if(sendQueue->size() > 0){
					server->sendPost(session);
				} else {
					session->_isSent = false;
				}
			
			}
			
			// recv �Ϸ� ó��
			else if(&session->_recvOverlapped == overlapped){

				CRingBuffer* recvBuffer = &session->_recvBuffer;

				recvBuffer->moveRear(transferred);

				// packet proc
				server->checkCompletePacket(session, recvBuffer);

				server->recvPost(session);
			
			}

		} while (false);

		} LeaveCriticalSection( &session->_lock );
	}

	return 0;
}

unsigned CLanServer::acceptFunc(void* args){
	
	CLanServer* server = (CLanServer*)args;
	SOCKET listenSocket = server->_listenSocket;

	HANDLE iocp = server->_iocp;

	SOCKADDR_IN addr;
	int addrLen = sizeof(SOCKADDR_IN);

	for(;;){

		SOCKET sock = accept(listenSocket, (SOCKADDR*)&addr, &addrLen);
		
		DWORD stopEventResult = WaitForSingleObject(server->_stopEvent, 0);
		if(WAIT_OBJECT_0 == stopEventResult){
			// thread stop
			break;
		}

		unsigned int ip = addr.sin_addr.S_un.S_addr;
		unsigned short port = addr.sin_port;
		
		/////////////////////////////////////////////////////////
		// ���� ���� ���� üũ
		/////////////////////////////////////////////////////////
		if(server->onConnectRequest(ip, port) == false){
			// ������ �źεǾ����ϴ�.
			closesocket(sock);
			continue;
		}
		/////////////////////////////////////////////////////////
		
		/////////////////////////////////////////////////////////
		// ���� ���
		/////////////////////////////////////////////////////////
		{
			unsigned __int64 sessionNum = server->_sessionNum;
			unsigned __int64* sessionCnt = &server->_sessionCnt;
			bool isSessionFull = sessionNum == *sessionCnt;
			
			/////////////////////////////////////////////////////////
			// ���� �ִ�ġ üũ
			/////////////////////////////////////////////////////////
			if(isSessionFull == true){
				server->onError(20000, L"���õ� ���������� �ִ�ġ �����Ͽ� �ű� ���� �Ұ�");
				closesocket(sock);
				continue;
			}
			/////////////////////////////////////////////////////////
			
			/////////////////////////////////////////////////////////
			// ���� �ʱ�ȭ
			/////////////////////////////////////////////////////////
			
			// session array�� index Ȯ��
			unsigned short sessionIndex = 0;
			CStack<int>* sessionIndexStack = server->_sessionIndexStack;
			sessionIndexStack->front((int*)&sessionIndex);
			sessionIndexStack->pop();

			// session Ȯ��
			stSession* sessionArr = server->_sessionArr;
			stSession* session = &sessionArr[sessionIndex];

			EnterCriticalSection(&session->_lock); {

				unsigned __int64 sessionID = session->_sessionID;
				unsigned __int64 sessionUseCnt = 0;

				// ID�� ���� 6����Ʈ�� ���� �޸𸮿� ���� ���� Ƚ��
				// ���� 2����Ʈ�� ���� �ε���
				
				// session id ����
				if(sessionID == 0){
					// ���� ���� ���
					int sendBufferSize = server->_sendBufferSize;
					int recvBufferSize = server->_recvBufferSize;

					new (session) stSession(sendBufferSize, recvBufferSize);

					session->_sessionID = (unsigned __int64)session;

				} else {
					// ���� ����
					sessionUseCnt = sessionID & SESSION_ALLOC_COUNT_MASK;

				}
			
				sessionID = (sessionUseCnt + 0x10000) | (unsigned __int64)sessionIndex;
				session->_sessionID = sessionID;
				session->_ip = ip;
				session->_port = port;
				session->_isSent = false;
				session->_sock = sock;		
				session->_beRelease = false;

				CRingBuffer* recvBuffer = &session->_recvBuffer;
				CQueue<CPacketPointer>* sendQueue = &session->_sendQueue;

				server->_sessionCnt += 1;

				CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)sessionID, 0);
			
				server->onClientJoin(ip, port, sessionID);

				server->recvPost(session);

			} LeaveCriticalSection(&session->_lock);

			/////////////////////////////////////////////////////////

		}
		/////////////////////////////////////////////////////////
	}

	return 0;
}

void CLanServer::recvPost(stSession* session){
	
	unsigned __int64 sessionID = session->_sessionID;

	/////////////////////////////////////////////////////////
	// recv buffer ������ wsa buf�� ����
	/////////////////////////////////////////////////////////
	WSABUF wsaBuf[2];
	int wsaCnt = 1;

	CRingBuffer* recvBuffer = &session->_recvBuffer;

	int rear = recvBuffer->rear();
	int front = recvBuffer->front();
	char* directPushPtr = recvBuffer->getDirectPush();
	int directFreeSize = recvBuffer->getDirectFreeSize();
	char* bufStartPtr = recvBuffer->getBufferStart();

	wsaBuf[0].buf = directPushPtr;
	wsaBuf[0].len = directFreeSize;

	if(front <= rear){
		wsaBuf[1].buf = bufStartPtr;
		wsaBuf[1].len = front;
		wsaCnt = 2;
	}
	/////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////
	// wsa recv
	/////////////////////////////////////////////////////////
	OVERLAPPED* overlapped = &session->_recvOverlapped;
	SOCKET sock = session->_sock;
	
	int recvResult;
	int recvError;

	unsigned int flag = 0;
	recvResult = WSARecv(sock, wsaBuf, wsaCnt, nullptr, (LPDWORD)&flag, overlapped, nullptr);
	if(recvResult == SOCKET_ERROR){
		recvError = WSAGetLastError();
		if(recvError != WSA_IO_PENDING){

			disconnect(sessionID);
			if(recvError != 10054){
				_log(L"recv.txt", LOG_GROUP::LOG_DEBUG, L"session: 0x%I64x, sock: %I64d, wsaCnt: %d, wsaBuf[0]: 0x%I64x, wsaBuf[1]: 0x%I64x\n", sessionID, sock, wsaCnt, wsaBuf[0], wsaBuf[1]);
			}

			return ;
		}
	}
	/////////////////////////////////////////////////////////
}

void CLanServer::sendPost(stSession* session){
	
	unsigned __int64 sessionID = session->_sessionID;

	/////////////////////////////////////////////////////////
	// ���� �����Ͱ� �ִ��� üũ
	/////////////////////////////////////////////////////////
	CQueue<CPacketPointer>* sendQueue = &session->_sendQueue;
	int wsaNum;

	unsigned int usedSize = sendQueue->size();
	wsaNum = usedSize;
	wsaNum = min(wsaNum, MAX_PACKET);
	/////////////////////////////////////////////////////////

	
	/////////////////////////////////////////////////////////
	// send 1ȸ ���� ó��
	/////////////////////////////////////////////////////////
	session->_isSent = true;
	/////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////
	// packet�� wsaBuf�� ����
	/////////////////////////////////////////////////////////
	WSABUF wsaBuf[MAX_PACKET];
	session->_packetCnt = wsaNum;
	int packetNum = wsaNum;

	CPacketPointer packet;

	for(int packetCnt = 0; packetCnt < packetNum; ++packetCnt){
		
		sendQueue->front(&packet);
		sendQueue->pop();
		wsaBuf[packetCnt].buf = packet.getBufStart();
		wsaBuf[packetCnt].len = packet.getPacketSize();
		packet.decRef();

		session->_packets[packetCnt] = packet;

	}
	/////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////
	// wsa send
	/////////////////////////////////////////////////////////
	int sendResult;
	int sendError;

	SOCKET sock = session->_sock;
	OVERLAPPED* overlapped = &session->_sendOverlapped;

	sendResult = WSASend(sock, wsaBuf, wsaNum, nullptr, 0, overlapped, nullptr);

	if(sendResult == SOCKET_ERROR){
		sendError = WSAGetLastError();
		if(sendError != WSA_IO_PENDING){
			disconnect(sessionID);
			session->_isSent = false;
			_log(L"send.txt", LOG_GROUP::LOG_SYSTEM, L"session: 0x%I64x, sock: %I64d, wsaNum: %d, wsaBuf[0]: 0x%I64x, wsaBuf[1]: 0x%I64x, error: %d\n", sessionID, sock, wsaNum, wsaBuf[0], wsaBuf[1], sendError);
			return ;
		}
	}	
	/////////////////////////////////////////////////////////

}

void CLanServer::start(const wchar_t* serverIP, unsigned short serverPort,
			int createWorkerThreadNum, int runningWorkerThreadNum,
			unsigned short maxSessionNum, bool onNagle,
			unsigned int sendBufferSize, unsigned int recvBufferSize){
		
	_sendBufferSize = sendBufferSize;
	_recvBufferSize = recvBufferSize;
	_sessionNum = maxSessionNum;

	// wsa startup
	int startupResult;
	int startupError;
	{
		WSAData wsaData;
		startupResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if(startupResult != NULL){
			startupError = WSAGetLastError();
			onError(startupError, L"wsa startup error");
			return ;
		}
	}

	// ���� ���� ����
	int socketError;
	{
		_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if(_listenSocket == INVALID_SOCKET){
			socketError = WSAGetLastError();
			onError(socketError, L"listen socket create error");
			return ;
		}
	}

	// ���� ���Ͽ� linger �ɼ� ����
	int lingerResult;
	int lingerError;
	{
		LINGER lingerSet = {(unsigned short)1, (unsigned short)0};
		lingerResult = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerSet, sizeof(LINGER));
		if(lingerResult == SOCKET_ERROR){
			lingerError = WSAGetLastError();
			onError(lingerError, L"listen socket ligner option error");
			return ;
		}
	}

	// ���� ���� send buffer 0
	int setSendBufferResult;
	int setSendBufferError;
	{
		int sendBufferSize = 0;
		setSendBufferResult = setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, sizeof(int));
		if(setSendBufferResult == SOCKET_ERROR){

			setSendBufferError = WSAGetLastError();
			onError(setSendBufferError, L"listen socket send buffer size ���� error");
			return ;
		}
	}
	
	// ���� ���� ���ε�
	int bindResult;
	int bindError;
	{
		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		InetPtonW(AF_INET, serverIP, &addr.sin_addr.S_un.S_addr);
		addr.sin_port = htons(serverPort);

		bindResult = bind(_listenSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
		if(bindResult == SOCKET_ERROR){
			bindError = WSAGetLastError();
			onError(bindError, L"listen socket bind error");
			return ;
		}

	}

	// ������ ����
	int listenResult;
	int listenError;
	{
		listenResult = listen(_listenSocket, SOMAXCONN);
		if(listenResult == SOCKET_ERROR){
			listenError = WSAGetLastError();
			onError(listenError, L"listen error");
			return ;
		}
	}

	// session �迭 �ʱ�ȭ
	{
		_sessionArr = (stSession*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(stSession) * _sessionNum);
		new (_sessionIndexStack) CStack<int>(_sessionNum);
		for(int sessionCnt = _sessionNum - 1; sessionCnt >= 0 ; --sessionCnt){
			new (&_sessionArr[sessionCnt]) stSession(sendBufferSize, recvBufferSize);
			_sessionIndexStack->push(sessionCnt);
		}
	}


	// iocp �ʱ�ȭ
	int iocpError;
	{
		_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, runningWorkerThreadNum);
		if(_iocp == NULL){
			iocpError = GetLastError();
			onError(iocpError, L"create iocp error");
			return ;
		}
	}

	// tps thread �ʱ�ȭ
	{
		_tpsCalcThread = (HANDLE)_beginthreadex(nullptr, 0, CLanServer::tpsCalcFunc, this, 0, nullptr);
	}

	// worker thread �ʱ�ȭ
	{
		_workerThreadNum = createWorkerThreadNum;
		_workerThread = (HANDLE*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(HANDLE) * createWorkerThreadNum);

		for(int threadCnt = 0; threadCnt < createWorkerThreadNum ; ++threadCnt){
			_workerThread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, CLanServer::completionStatusFunc, (void*)this, 0, nullptr);
		}

	}

	// accept thread �ʱ�ȭ
	{
		_acceptThread = (HANDLE)_beginthreadex(nullptr, 0, CLanServer::acceptFunc, (void*)this, 0, nullptr);
	}

}

void CLanServer::checkCompletePacket(stSession* session, CRingBuffer* recvBuffer){
	
	unsigned __int64 sessionID = session->_sessionID;
	unsigned int usedSize = recvBuffer->getUsedSize();

	while(usedSize > sizeof(stHeader)){
		
		// header üũ
		stHeader header;
		recvBuffer->frontBuffer(sizeof(stHeader), (char*)&header);

		int payloadSize = header.size;
		int packetSize = payloadSize + sizeof(stHeader);

		// ��Ŷ�� recvBuffer�� �ϼ��Ǿ��ٸ�
		if(usedSize >= packetSize){
			
			recvBuffer->popBuffer(sizeof(stHeader));

			CPacketPtr_Lan packet;

			recvBuffer->frontBuffer(payloadSize, packet.getRearPtr());
			packet.moveRear(payloadSize);

			recvBuffer->popBuffer(payloadSize);
			
			unsigned short sessionIndex = sessionID & SESSION_INDEX_MASK;
			stSession* session = &_sessionArr[sessionIndex];

			packet.moveFront(sizeof(stHeader));

			InterlockedIncrement((LONG*)&_recvCnt);
			onRecv(sessionID, packet);
			packet.decRef();

			usedSize -= packetSize;

		} else {
			break;
		}

	}
}

void CLanServer::stop(){

	closesocket(_listenSocket);

	bool setStopEventResult = SetEvent(_stopEvent);
	int setEventError;
	if(setStopEventResult == false){
		setEventError = GetLastError();
		CDump::crash();
	}

	WaitForSingleObject(_acceptThread, INFINITE);

	for(int sessionCnt = 0; sessionCnt < _sessionNum ; ++sessionCnt){
		disconnect(_sessionArr[sessionCnt]._sessionID);
	}
	while(_sessionCnt > 0);

	for(int threadCnt = 0 ; threadCnt < _workerThreadNum; ++threadCnt){
		PostQueuedCompletionStatus((HANDLE)_iocp, (DWORD)0, (ULONG_PTR)0, (LPOVERLAPPED)0);
	}
	WaitForMultipleObjects(_workerThreadNum, _workerThread, true, INFINITE);

	CloseHandle(_iocp);
	
	_sessionIndexStack->~CStack();
	for(int sessionCnt = 0; sessionCnt < _sessionNum; ++sessionCnt){
		_sessionArr[sessionCnt].~stSession();
	}
	HeapFree(_heap, 0, _sessionArr);
	HeapFree(_heap, 0, _workerThread);
}

unsigned __stdcall CLanServer::tpsCalcFunc(void* args){

	CLanServer* server = (CLanServer*)args;

	int* sendCnt = &server->_sendCnt;
	int* recvCnt = &server->_recvCnt;
	int* sendTPS = &server->_sendTPS;
	int* recvTPS = &server->_recvTPS;

	for(;;){

		*sendTPS = *sendCnt;
		*recvTPS = *recvCnt;

		InterlockedExchange((LONG*)sendCnt, 0);
		InterlockedExchange((LONG*)recvCnt, 0);

		Sleep(999);

	}

	return 0;
}



CLanServer::stSession::stSession(unsigned int sendQueueSize, unsigned int recvBufferSize):
	_recvBuffer(recvBufferSize), _sendQueue(sendQueueSize)
{
	_sessionID = 0;
	_sock = NULL;
	_ip = 0;
	_port = 0;
	_isSent = false;
	_beRelease = false;

	ZeroMemory(&_sendOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(&_recvOverlapped, sizeof(OVERLAPPED));
	_packets = (CPacketPointer*)HeapAlloc(_heap, HEAP_ZERO_MEMORY, sizeof(CPacketPointer) * MAX_PACKET);
	_packetCnt = 0;

	InitializeCriticalSectionAndSpinCount(&_lock, 0);

}

CLanServer::stSession::~stSession(){
	HeapFree(_heap, 0, _packets);
	DeleteCriticalSection(&_lock);
}