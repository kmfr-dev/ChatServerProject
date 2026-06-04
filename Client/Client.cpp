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

	// 수신 전용 이벤트 객체 생성	
	mRecvBuffer.Overlapped.hEvent = WSACreateEvent();
	mRecvBuffer.rwMode = IO_MODE::READ;

	// 메시지 매니저 쓰레드 초기화
	CChatServerApp::GetInstance()->GetMessageManager()->InitRecvThread();

	// 서버에 자신의 이름 메세지 보내기
	SendChatMessage(EChatType::CHAT_TYPE_CONNECTED, mName);
}

void CClient::RegisterRecv()
{
	mRecvBuffer.WSABuf.buf = mRecvBuffer.Buffer;
	mRecvBuffer.WSABuf.len = sizeof(FChatPacket);
	ZeroMemory(&mRecvBuffer.Overlapped, sizeof(WSAOVERLAPPED));

	DWORD flag = 0;

	int result = WSARecv(mConnectedSocket, &mRecvBuffer.WSABuf, 1, 
		nullptr, &flag, &mRecvBuffer.Overlapped, &CMessageManager::RecvCallBack);

	if (SOCKET_ERROR == result)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			std::cerr << "WSARecv Error : " + std::to_string(error) << std::endl;
		}
	}
}


void CClient::End()
{
	shutdown(mConnectedSocket, SD_BOTH);
	closesocket(mConnectedSocket);
	WSACleanup();
}

void CClient::SendChatMessage(EChatType _Type, const std::string& _Message)
{
	// 송신 버퍼 동적 할당, 완료되기 전에 스택에서 사라지기 때문에
	FBufferInfo* sendData = new FBufferInfo;
	sendData->rwMode = IO_MODE::WRITE;

	FChatPacket* packet = (FChatPacket*)sendData->Buffer;
	packet->Type = _Type;
	strncpy_s(packet->Message, _Message.c_str(), PACKET_SIZE);

	sendData->WSABuf.buf = sendData->Buffer;
	sendData->WSABuf.len = PACKET_SIZE;

	DWORD sendBytes = 0;
	
	// 비동기 송신
	if (SOCKET_ERROR == WSASend(mConnectedSocket, &sendData->WSABuf, 1, 
		&sendBytes, 0, &sendData->Overlapped, &CMessageManager::SendCallBack))
	{
		// 에러 발생시 이벤트 정리 및 동적 할당된 송신버퍼 해제
		int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			WSACloseEvent(sendData->Overlapped.hEvent);

			delete sendData;

			CChatServerApp::GetInstance()->GetMessageManager()->
				AddChat(EChatType::CHAT_TYPE_ERROR, "Send Error : " + std::to_string(errorCode), false);
		}
	}
}