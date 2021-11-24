#include "lanServer.h"

CLanServer::CLanServer(){
	
	_sessionFreeList = nullptr;

	_sessionNum = 0;
	_sessionCnt = 0;

	_acceptThread = NULL;
	_workerThread = nullptr;

	_sendBufferSize = 0;
	_recvBufferSize = 0;

	_listenSocket = NULL;
	_iocp = NULL;

	_sessionPtrMask = 0x000007FFFFFFFFFF;
	_sessionIDMask  = 0xFFFFF80000000000;
}

bool CLanServer::disconnect(unsigned __int64 sessionID){
	
	stSession* session = (stSession*)(sessionID & _sessionPtrMask);

	SOCKET sock = session->_sock;

	closesocket(sock);

	session->_beDisconnect = true;

	return true;

}

bool CLanServer::sendPacket(unsigned __int64 sessionID, CProtocolBuffer* packet){

	stSession* session = (stSession*)(sessionID & _sessionPtrMask);
	CRingBuffer* sendBuffer = &session->_sendBuffer;

	stHeader header;
	unsigned short packetSize = (unsigned short)packet->getUsedSize();
	char* packetBuf = packet->getFrontPtr();
	header.size = packetSize;

	unsigned int freeSize = sendBuffer->getFreeSize();

	sendBuffer->pushLock(); {

		sendBuffer->push(sizeof(stHeader), (char*)&header);
		sendBuffer->push(packetSize, packetBuf);

	} sendBuffer->pushUnlock();

	bool isSent = session->_isSent;
	if(isSent == false){
		sendPost(session);
	}

	return true;

}

unsigned CLanServer::completionStatusFunc(void *args){
	
	CLanServer* server = (CLanServer*)args;

	HANDLE iocp = server->_iocp;

	while(1){
		
		unsigned int transferred;
		stSession* session;
		OVERLAPPED *overlapped;
		GetQueuedCompletionStatus(iocp, (LPDWORD)&transferred, (PULONG_PTR)&session, &overlapped, INFINITE);

		if(&session->_sendOverlapped == overlapped){
			// send 완료

			CRingBuffer *sendBuffer = &session->_sendBuffer;
			//wprintf(L"sent %d\n", session->_ioCnt);

			sendBuffer->popLock(); {

				sendBuffer->pop(transferred);

				bool *isSent = &session->_isSent;
				
				*isSent = false;

				
			} sendBuffer->popUnlock();
			
			unsigned int usedSize = sendBuffer->getUsedSize();
			if(usedSize > 0){

				server->sendPost(session);

			}


		}

		if(&session->_recvOverlapped == overlapped){
			// recv 완료
			
			//wprintf(L"recved %d\n", session->_ioCnt);

			unsigned __int64 sessionID = session->_sessionID;
			CRingBuffer *recvBuffer = &session->_recvBuffer;

			recvBuffer->moveRear(transferred);

			// packet proc
			server->checkCompletePacket(sessionID, recvBuffer);

			server->recvPost(session);
			
		}

		short* ioCnt = &session->_ioCnt;
		InterlockedDecrement16(ioCnt);

		if(*ioCnt == 0 && session->_beDisconnect == true){
			//wprintf(L"free %d\n", session->_ioCnt);
			server->sessionFreeListLock();{
				server->_sessionFreeList->freeObject(session);
				server->_sessionCnt -= 1;
				wprintf(L"disconnect: %d\n", session->_sessionID);
			} server->sessionFreeListUnlock();
		}

	}

	return 0;
}

unsigned CLanServer::acceptFunc(void* args){
	
	CLanServer* server = (CLanServer*)args;
	SOCKET listenSocket = server->_listenSocket;

	HANDLE iocp = server->_iocp;

	while(1){

		SOCKADDR_IN addr;
		int addrLen = sizeof(SOCKADDR_IN);
		SOCKET sock = accept(listenSocket, (SOCKADDR*)&addr, &addrLen);
				
		unsigned int ip = addr.sin_addr.S_un.S_addr;
		unsigned short port = addr.sin_port;

		if(server->onConnectRequest(ip, port) == false){
			// 접속이 거부되었습니다.
			closesocket(sock);
			continue;
		}

		// 접속이 허용되었습니다.
		{
			unsigned __int64 sessionNum = server->_sessionNum;
			unsigned __int64* sessionCnt = &server->_sessionCnt;
			bool isSessionFull = sessionNum == *sessionCnt;
			
			if(isSessionFull == true){
				// 서버 동접 최대치에 도달했습니다.
				server->onError(SERVER_ERROR::SESSION_FULL, L"서버에 세팅된 동시접속자 최대치에 도달하여, 새로운 연결을 받을 수 없습니다.");
				closesocket(sock);
				continue;
			}

			// 서버에서 새로운 새션을 받을 수 있습니다.
			// 세션 초기화 진행합니다.
			CObjectFreeList<stSession>* sessionFreeList;
			server->sessionFreeListLock(); {
				sessionFreeList = server->_sessionFreeList;
				*sessionCnt += 1;
			} server->sessionFreeListUnlock();
			stSession* session = sessionFreeList->allocObject();
			unsigned __int64 sessionID = session->_sessionID;

			// ID의 상위 21비트는 세션 메모리에 대한 재사용 횟수
			// 하위 43비트는 세션 메모리에 대한 주소

			if(sessionID == 0){

				int sendBufferSize = server->_sendBufferSize;
				int recvBufferSize = server->_recvBufferSize;

				new (session) stSession(sendBufferSize, recvBufferSize);

			} else {

				unsigned __int64 sessionIDMask = server->_sessionIDMask;

				sessionID &= sessionIDMask;

			}
			
			sessionID = ((sessionID >> 43) + 1) << 43;
			sessionID |= (unsigned __int64)session;

			session->_sessionID = sessionID;
			session->_ip = ip;
			session->_port = port;
			session->_isSent = false;
			session->_sock = sock;
			session->_beDisconnect = false;

			CRingBuffer* recvBuffer = &session->_recvBuffer;
			CRingBuffer* sendBuffer = &session->_sendBuffer;

			recvBuffer->pop(recvBuffer->getUsedSize());
			sendBuffer->pop(sendBuffer->getUsedSize());
						
			CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)session, 0);

			server->onClientJoin(ip, port, sessionID);

			server->recvPost(session);

		}
	}

	return 0;
}

void CLanServer::recvPost(stSession* session){
	

	short* ioCnt = &session->_ioCnt;
	InterlockedIncrement16(ioCnt);
	
	//wprintf(L"recv %d\n",*ioCnt);

	CRingBuffer* recvBuffer = &session->_recvBuffer;
	SOCKET sock = session->_sock;
	OVERLAPPED* overlapped = &session->_recvOverlapped;
	unsigned __int64 sessionID = session->_sessionID;

	WSABUF wsaBuf[2];
	int wsaCnt = 1;

	int rear = recvBuffer->rear();
	int front = recvBuffer->front();
	char* directPushPtr = recvBuffer->getDirectPush();
	int directFreeSize = recvBuffer->getDirectFreeSize();
	int freeSize = recvBuffer->getFreeSize();
	char* bufStartPtr = recvBuffer->getBufferStart();

	wsaBuf[0].buf = directPushPtr;
	wsaBuf[0].len = directFreeSize;

	if(front <= rear){
		wsaBuf[1].buf = bufStartPtr;
		wsaBuf[1].len = front;
		wsaCnt = 2;
	}

	int recvResult;
	int recvError;
	{
		unsigned int flag = 0;
		recvResult = WSARecv(sock, wsaBuf, wsaCnt, nullptr, (LPDWORD)&flag, overlapped, nullptr);
		if(recvResult == SOCKET_ERROR){
			recvError = WSAGetLastError();
			if(recvError != WSA_IO_PENDING){
				InterlockedDecrement16(ioCnt);
				if(*ioCnt == 0){
					sessionFreeListLock();{
						_sessionFreeList->freeObject(session);
						_sessionCnt -= 1;
					} sessionFreeListUnlock();
				}
				disconnect(sessionID);

				if(recvError == 10054){
					wprintf(L"recv error: %d\n", recvError);
				}
				wprintf(L"recv fail, ioCnt: %d\n",*ioCnt);
				return ;
			}
		}
	} 

}

void CLanServer::sendPost(stSession* session){
	
	bool* sent = &session->_isSent;
	CRingBuffer* sendBuffer = &session->_sendBuffer;

	sendBuffer->popLock(); {
	
		short* ioCnt = &session->_ioCnt;
		InterlockedIncrement16(ioCnt);
		//wprintf(L"send %d\n",*ioCnt);

		if(*sent == true){
			//wprintf(L"send fail %d\n", *ioCnt);
			sendBuffer->popUnlock();
			InterlockedDecrement16(ioCnt);
			if(*ioCnt == 0){
				sessionFreeListLock();{
					_sessionFreeList->freeObject(session);
					_sessionCnt -= 1;
				} sessionFreeListUnlock();
			}
			return ;
		}
		*sent = true;


		SOCKET sock = session->_sock;
		OVERLAPPED* overlapped = &session->_sendOverlapped;
		unsigned __int64 sessionID = session->_sessionID;

		int usedSize = sendBuffer->getUsedSize();

		int front = sendBuffer->front();
		int rear = sendBuffer->rear();

		char* directFrontPtr = sendBuffer->getDirectFront();
		int directUsedSize = sendBuffer->getDirectUsedSize();
		
		char* bufStartPtr = sendBuffer->getBufferStart();

		WSABUF wsaBuf[2];
		int wsaCnt = 1;

		wsaBuf[0].buf = directFrontPtr;
		wsaBuf[0].len = directUsedSize;

		if(front > rear){
			wsaBuf[1].buf = bufStartPtr;
			wsaBuf[1].len = rear;
			wsaCnt = 2;
		}

		int sendResult;
		int sendError;
		{
			sendResult = WSASend(sock, wsaBuf, wsaCnt, nullptr, 0, overlapped, nullptr);
			if(sendResult == SOCKET_ERROR){
				sendError = WSAGetLastError();
				if(sendError != WSA_IO_PENDING){
					InterlockedDecrement16(ioCnt);
					if(*ioCnt == 0){
						sessionFreeListLock();{
							_sessionFreeList->freeObject(session);
							_sessionCnt -= 1;
						} sessionFreeListUnlock();
					}
					*sent = false;
					disconnect(sessionID);
					sendBuffer->popUnlock();
					wprintf(L"send error: %d\n", sendError);
					wprintf(L"send fail %d\n", *ioCnt);
					return ;
				}
			}
		}
	
	} sendBuffer->popUnlock();
}

void CLanServer::start(const wchar_t* configFileName){

	CStringParser settingParser(configFileName);

	// 세팅 파일에서 필요한 데이터 수집
	wchar_t* serverIP;
	unsigned short serverPort;
	int createWorkerThreadNum;
	int runningWorkerThreadNum;
	{
		settingParser.setNameSpace(L"lanServer");

		wchar_t buf[20];

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"serverIP");
		size_t len = wcslen(buf);
		serverIP = new wchar_t[len + 1];
		serverIP[len] = '\0';
		wmemcpy(serverIP, buf, len);
		
		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"serverPort");
		serverPort = (unsigned short)wcstol(buf, nullptr, 10);

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"createWorkerThreadNum");
		createWorkerThreadNum = wcstol(buf, nullptr, 10);

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"runningWorkerThreadNum");
		runningWorkerThreadNum = wcstol(buf, nullptr, 10);

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"maxSessionNum");
		_sessionNum = wcstol(buf, nullptr, 10);

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"sessionSendBufferSize");
		_sendBufferSize = wcstol(buf, nullptr, 10);

		ZeroMemory(buf, 20);
		settingParser.getValueByKey(buf, L"sessionRecvBufferSize");
		_recvBufferSize = wcstol(buf, nullptr, 10);

	}

	// wsa startup
	int startupResult;
	int startupError;
	{
		WSAData wsaData;
		startupResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if(startupResult != NULL){
			startupError = WSAGetLastError();
			stStartError error(startupError, L"wsa startup Error");
			throw error;
		}
	}

	// 리슨 소켓 생성
	int socketError;
	{
		_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if(_listenSocket == INVALID_SOCKET){
			socketError = WSAGetLastError();
			stStartError error(socketError, L"listen socket init Error");
			throw error;
		}
	}

	// 리슨 소켓에 linger 옵션 적용
	int lingerResult;
	int lingerError;
	{
		LINGER lingerSet = {(unsigned short)1, (unsigned short)0};
		lingerResult = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerSet, sizeof(LINGER));
		if(lingerResult == SOCKET_ERROR){
			lingerError = WSAGetLastError();
			stStartError error(lingerError, L"linger Error");
			throw error;
		}
	}
	
	// 리슨 소켓 바인딩
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
			stStartError error(bindError, L"bind Error");
			throw error;
		}

	}

	// 리스닝 시작
	int listenResult;
	int listenError;
	{
		listenResult = listen(_listenSocket, SOMAXCONN);
		if(listenResult == SOCKET_ERROR){
			listenError = WSAGetLastError();
			stStartError error(listenError, L"listen Error");
			throw error;
		}
	}

	// session 배열 초기화
	{
		_sessionFreeList = new CObjectFreeList<stSession>((int)_sessionNum);
	}


	// iocp 초기화
	int iocpError;
	{
		_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, runningWorkerThreadNum);
		if(_iocp == NULL){
			iocpError = GetLastError();
			stStartError error(iocpError, L"create io completion port Error");
			throw error;
		}
	}

	// worker thread 초기화
	{
		_workerThread = new HANDLE[createWorkerThreadNum];

		for(int threadCnt = 0; threadCnt < createWorkerThreadNum ; ++threadCnt){
			_workerThread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, CLanServer::completionStatusFunc, (void*)this, 0, nullptr);
		}

	}

	// accept thread 초기화
	{
		_acceptThread = (HANDLE)_beginthreadex(nullptr, 0, CLanServer::acceptFunc, (void*)this, 0, nullptr);
	}

}

unsigned __int64 CLanServer::getSessionCount(){
	
	return _sessionCnt;

	return 0;
	
}

void CLanServer::checkCompletePacket(unsigned __int64 sessionID, CRingBuffer* recvBuffer){
	
	unsigned int usedSize = recvBuffer->getUsedSize();

	while(usedSize > sizeof(stHeader)){
		
		stHeader header;

		recvBuffer->front(sizeof(stHeader), (char*)&header);

		int payloadSize = header.size;
		int packetSize = payloadSize + sizeof(stHeader);

		if(usedSize >= packetSize){
			
			recvBuffer->pop(sizeof(stHeader));

			CProtocolBuffer* buffer = new CProtocolBuffer(payloadSize);
			recvBuffer->front(payloadSize, buffer->getRearPtr());
			buffer->moveRear(payloadSize);

			recvBuffer->pop(payloadSize);
			onRecv(sessionID, buffer);

			usedSize -= packetSize;

		} else {
			break;
		}

	}
}

void CLanServer::sessionFreeListLock(){
	AcquireSRWLockExclusive(&_sessionFreeListLock);	
}

void CLanServer::sessionFreeListUnlock(){
	ReleaseSRWLockExclusive(&_sessionFreeListLock);
}