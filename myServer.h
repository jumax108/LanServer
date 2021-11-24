#pragma once

#include "lanServer.h"

class CMyServer : public CLanServer{
	
public :
	// Ŭ���̾�Ʈ�� ������ �õ��� ���¿��� ȣ��˴ϴ�.
	// ��ȯ�� ���� ���� ������ ����մϴ�.
	// return true = ���� ��, ���� �ʱ�ȭ
	// return false = ������ ����
	virtual bool onConnectRequest(unsigned int ip, unsigned short port){
		return true;
	}
	// Ŭ���̾�Ʈ�� ������ �Ϸ��� ���¿��� ȣ��˴ϴ�.
	virtual void onClientJoin(unsigned int ip, unsigned short port, unsigned __int64 sessionID){
		return ;
	}
	// Ŭ���̾�Ʈ�� ������ �����Ǹ� ȣ��˴ϴ�.
	virtual void onClientLeave(unsigned __int64 sessionID){
		return ;
	}

	// Ŭ���̾�Ʈ���� �����͸� �����ϸ� ȣ��˴ϴ�.
	virtual void onRecv(unsigned __int64 sessionID, CProtocolBuffer* packet){
	
		PostQueuedCompletionStatus(_iocp, 0, sessionID, (LPOVERLAPPED)packet);
	}
	// Ŭ���̾�Ʈ���Լ� �����͸� ���޹����� ȣ��˴ϴ�.
	virtual void onSend(unsigned __int64 sessionID, int sendSize){	
	}

	// ���� ��Ȳ���� ȣ��˴ϴ�.
	virtual void onError(SERVER_ERROR errorCode, const wchar_t* errorMsg){
		wprintf(L"code: %d, msg: %s\n", errorCode, errorMsg);
	}
	
	HANDLE _iocp;

private:
	

};