#include "Client.h"
#include "ChatServerApp.h"
#include "MessageManager.h"

bool CClient::Init()
{
	// 디버그 일때만 이름, IP 강제 지정
#ifdef _DEBUG
	mName = "Dummy Client";
	mServerIP = "127.0.0.1";
#endif 

	return true;
}

void CClient::ConnectToServer()
{
	WSADATA wsa = {};

	int wsaStartResult = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (0 != wsaStartResult)
	{
		std::cerr << "WSAStartup failed.. error code : " << wsaStartResult << std::endl;
		return;
	}

	// 클라 소켓 생성
	mConnectedSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mConnectedSocket)
	{
		std::cerr << "Connected failed.." << std::endl;
		return;
	}

	SOCKADDR_IN addr = { };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, mServerIP.c_str(), &addr.sin_addr.s_addr);

	if (connect(mConnectedSocket, (SOCKADDR*)&addr, sizeof(addr)))
	{
		std::cerr << "Connected failed.." << std::endl;
		return;
	}

	if (INVALID_SOCKET == mConnectedSocket)
	{
		std::cerr << "Connected Socket Is Invalid.." << std::endl;
		return;
	}

	// 수신 전용 이벤트 객체 생성	
	mRecvBuffer.Overlapped.hEvent = WSACreateEvent();
	mRecvBuffer.rwMode = IO_MODE::READ;

	// 메시지 매니저 쓰레드 초기화
	CChatServerApp::GetInstance()->GetMessageManager()->InitRecvThread();

	// 서버에 자신의 이름 메세지 보내기
	CChatServerApp::GetInstance()->GetMessageManager()->
		AddChat(EChatType::CHAT_TYPE_CONNECTED, mName, true);
}

void CClient::End()
{
	// 릴리즈일때는 클라 소켓 하나만 닫기
#ifndef _DEBUG
	shutdown(mConnectedSocket, SD_BOTH);
	closesocket(mConnectedSocket);
#else
	// TODO RELEASE DUMMYCLIENT

#endif

	WSACleanup();
}