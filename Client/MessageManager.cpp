#include "MessageManager.h"
#include "ChatServerApp.h"
#include "Client.h"

bool CMessageManager::Init()
{
	return true;
}

void CMessageManager::End()
{
	if (mThread.joinable())
	{
		mThread.join();
	}
}

void CMessageManager::InitRecvThread()
{
	mThread = std::thread(&CMessageManager::RecvChat, this);
}

void CMessageManager::RecvChat()
{
	CClient* client = CChatServerApp::GetInstance()->GetClient();
	if (nullptr == client)
		return;

	FChatPacket packet;

	//SOCKET sock = client->GetSocket();
	//// 클라의 수신버퍼
	//FBufferInfo& recvBuffer = client->GetRecvBuffer();

	//WSAEVENT eventArr[1] = { recvBuffer.Overlapped.hEvent };

	while (true)
	{
		int result = recv(client->GetSocket(), (char*)&packet, sizeof(packet), 0);
		if (result > 0)
		{
			AddChat(packet.Type, packet.Message, false);
		}


		//DWORD result = WSAWaitForMultipleEvents(1, eventArr, FALSE, WSA_INFINITE, FALSE);

		//if (WSA_WAIT_FAILED == result)
		//	break;
		//	
		//WSAResetEvent(eventArr[0]);

		//DWORD bytes = 0;
		//DWORD flag = 0;

		//if (WSAGetOverlappedResult(sock, &recvBuffer.Overlapped, &bytes, FALSE, &flag))
		//{
		//	// 연결 종료 신호
		//	if (0 == bytes)
		//	{
		//		AddChat(EChatType::CHAT_TYPE_ERROR, "서버 연결 종료..", false);
		//		break;
		//	}

		//	IO_MODE IOMode = recvBuffer.rwMode;
		//	FChatPacket* recvPacket = (FChatPacket*)recvBuffer.Buffer;

		//	// 채팅 패킷 처리
		//	AddChat(recvPacket->Type, recvPacket->Message, false);
		//	
		//	if (IO_MODE::READ == IOMode)
		//	{
		//		client->RegisterRecv();
		//	}
		//	else if (IO_MODE::WRITE == IOMode)
		//	{
		//		FBufferInfo* BufInfo = (FBufferInfo*)&recvBuffer.Overlapped;
		//		delete BufInfo;
		//		BufInfo = nullptr;
		//	}			
		//}

		//else
		//{
		//	// 에러 처리
		//	int errorCode = WSAGetLastError();
		//	AddChat(EChatType::CHAT_TYPE_ERROR, "Recv Error : " + std::to_string(errorCode), false);
		//	break;
		//}
	}
}

void CMessageManager::AddChat(EChatType _Type, const std::string& _Message, bool _IsSend)
{
	CClient* client = CChatServerApp::GetInstance()->GetClient();
	if (nullptr == client)
		return;

	if (_IsSend)
	{
		client->SendChatMessage(_Type, _Message);
		return;
	}
	
	mChatMutex.lock();

	FChatData data(_Type, _Message);

	mChats.push_back(data);

	mChatMutex.unlock();

	EraseOldedChat();
}

void CMessageManager::EraseOldedChat()
{
	mChatMutex.lock();

	// 채팅 배열이 20보다 크다면 가장 예전 채팅 메세지를 제거
	if (mChats.size() > 20)
	{
		mChats.erase(mChats.begin());
	}

	mChatMutex.unlock();
}
