#include "Client.h"
#include "ChatServerApp.h"
#include "MessageManager.h"

bool CClient::Init()
{
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

	mRecvBuffer.rwMode = IO_MODE::READ;

	// 서버에 자신의 이름 메세지 보내기
	CChatServerApp::GetInstance()->GetMessageManager()->
		AddChat(EChatType::CHAT_TYPE_CONNECTED, mName, true);
}

void CClient::End()
{
	shutdown(mConnectedSocket, SD_BOTH);
	closesocket(mConnectedSocket);
#ifndef _DEBUG
	WSACleanup();
#endif
}