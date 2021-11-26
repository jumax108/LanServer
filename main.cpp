#include <stdio.h>

#include "myServer.h"

unsigned __stdcall echoFunc(void *);

int main(){

	CMyServer server;
	
	server._iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	_beginthreadex(nullptr, 0, echoFunc, &server, 0, nullptr);
	_beginthreadex(nullptr, 0, echoFunc, &server, 0, nullptr);

	try{
		server.start(L"serverConfig.txt");
	} catch(CLanServer::stStartError error){
		wprintf(L"code: %d\nmsg: %s\n", error._errorCode, error._errorMessage);
	}

	wprintf(L"server Start\n");



	while(1){
		
		

	}

	return 0;

}

unsigned __stdcall echoFunc(void* args){
	
	CMyServer* server = (CMyServer*)args;

	while(1){

		DWORD transferred;

		unsigned __int64 sessionID;
		CProtocolBuffer* packet;

		GetQueuedCompletionStatus(server->_iocp, &transferred, &sessionID, (LPOVERLAPPED*)&packet, INFINITE);

		server->sendPacket(sessionID, packet);

		delete packet;

	}
}