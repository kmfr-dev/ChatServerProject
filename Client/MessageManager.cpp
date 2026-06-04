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
	// 송/수신 쓰레드가 처음에 수신등록.
	// Overlapped I/O 콜백 방식은 비동기 함수를 호출한 
	// 쓰레드가 대기 상태로 들어가야 콜백이 실행됨.
	RegisterRecv();

	while (true)
	{
		// 콜백을 호출하려면
		// 이 쓰레드를 Alertable Wait 상태로 진입해야 함.
		// 데이터 송수신 완료가 될때 까지 쓰레드는 대기
		SleepEx(INFINITE, TRUE);
	}
}

void CMessageManager::AddChat(EChatType _Type, const std::string& _Message, bool _IsSend)
{
	if (_IsSend)
	{
		StartChatSend(_Type, _Message);
		return;
	}
	
	mChatMutex.lock();

	FChatData data(_Type, _Message);

	mChats.emplace_back(data);

	mChatMutex.unlock();

	EraseOldedChat();
}

void CMessageManager::EraseOldedChat()
{
	mChatMutex.lock();

	// 채팅 배열이 최대 크기 보다 크다면 
	// 가장 예전 채팅 메세지를 제거
	if (mChats.size() > MAX_CHATSIZE)
	{
		mChats.pop_front();
	}

	mChatMutex.unlock();
}

void CMessageManager::RegisterRecv()
{
	CClient* client = CChatServerApp::GetInstance()->GetClient();
	if (nullptr == client)
	{
		std::cerr << "Client nullptr..!"  << std::endl;
		return;
	}

	client->GetMutex().lock();
	
	FBufferInfo& bufferInfo = client->GetRecvBuffer();
	bufferInfo.WSABuf.buf = bufferInfo.Buffer;
	bufferInfo.WSABuf.len = sizeof(FChatPacket);
	ZeroMemory(&bufferInfo.Overlapped, sizeof(WSAOVERLAPPED));
	
	DWORD flag = 0;

	// 비동기 수신 요청
	int result = WSARecv(client->GetSocket(), &bufferInfo.WSABuf, 1,
		nullptr, &flag, &bufferInfo.Overlapped, &RecvCallBack);

	if (SOCKET_ERROR == result)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			std::cerr << "WSARecv Error : " + std::to_string(error) << std::endl;
		}
	}

	client->GetMutex().unlock();
}

void CMessageManager::StartChatSend(EChatType _Type, const std::string& _Message)
{
	// 송신 버퍼 동적 할당, 완료되기 전에 스택에서 사라지기 때문에
	FBufferInfo* sendData = new FBufferInfo;
	sendData->rwMode = IO_MODE::WRITE;

	FChatPacket* packet = (FChatPacket*)sendData->Buffer;
	packet->Type = _Type;
	strncpy_s(packet->Message, _Message.c_str(), PACKET_SIZE);

	sendData->WSABuf.buf = sendData->Buffer;
	sendData->WSABuf.len = PACKET_SIZE;

	mChatMutex.lock();

	mSendQueue.push(sendData);

	mChatMutex.unlock();

	// 실행흐름을 메인 -> 송수신 쓰레드로 넘김
	QueueUserAPC(&CMessageManager::RequestAsyncSend, (HANDLE)mThread.native_handle(), (ULONG_PTR)this);
}

void CALLBACK CMessageManager::RecvCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 버퍼 정보 구조체로 변환
	FBufferInfo* Info = (FBufferInfo*)_Overlapped;

	CMessageManager* messageManager = CChatServerApp::GetInstance()->GetMessageManager();

	// 에러 및 서버 종료라면
	if (nullptr == messageManager || _Error || _Bytes == 0)
	{
		CChatServerApp::GetInstance()->GetMessageManager()->
			AddChat(EChatType::CHAT_TYPE_ERROR, "서버와 연결이 끊겼습니다..", false);
		return;
	}

	// 수신된 패킷을 처리
	FChatPacket* packet = (FChatPacket*)Info->Buffer;
	messageManager->AddChat(packet->Type, packet->Message, false);

	// 클라이언트는 다음 데이터를 받기 위해 수신 등록
	messageManager->RegisterRecv();
}

void CALLBACK CMessageManager::RequestAsyncSend(ULONG_PTR _Ptr)
{
	CMessageManager* messageManager = (CMessageManager*)_Ptr;

	messageManager->mChatMutex.lock();
	
	std::queue<FBufferInfo*>& sendQueue = messageManager->mSendQueue;

	while (!sendQueue.empty())
	{
		FBufferInfo* sendData = sendQueue.front();
		sendQueue.pop();

		DWORD sendBytes = 0;
		SOCKET clientSock = CChatServerApp::GetInstance()->GetClient()->GetSocket();

		// 실제로 비동기 수신 요청.
		WSASend(clientSock, &sendData->WSABuf, 1, &sendBytes, 0, &sendData->Overlapped, &CMessageManager::SuccessAsyncSend);
	}

	messageManager->mChatMutex.unlock();
}

void CMessageManager::SuccessAsyncSend(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 송신 콜백이면, 
	// 이전에 동적할당된 버퍼 정보이므로 메모리만 해제한다.
	FBufferInfo* bufferInfo = (FBufferInfo*)_Overlapped;

	delete bufferInfo;
}
