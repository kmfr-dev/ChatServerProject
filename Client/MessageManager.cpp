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
	CChatServerApp::GetInstance()->GetClient()->RegisterRecv();


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

void CALLBACK CMessageManager::RecvCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 버퍼 정보 구조체로 변환
	FBufferInfo* Info = (FBufferInfo*)_Overlapped;
	CClient* client = CChatServerApp::GetInstance()->GetClient();

	// 에러 및 서버 종료라면
	if (nullptr == client || _Error || _Bytes == 0)
	{
		CChatServerApp::GetInstance()->GetMessageManager()->
			AddChat(EChatType::CHAT_TYPE_ERROR, "서버와 연결이 끊겼습니다..", false);
		return;
	}

	// 수신된 패킷을 처리
	FChatPacket* packet = (FChatPacket*)Info->Buffer;
	CChatServerApp::GetInstance()->GetMessageManager()->AddChat(packet->Type, packet->Message, false);

	// 클라이언트는 다음 데이터를 받기 위해 수신 등록
	client->RegisterRecv();
}

void CALLBACK CMessageManager::SendCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 송신 콜백이면, 이전에 동적할당된 버퍼 정보이므로
	// 메모리만 해제한다.
	FBufferInfo* Info = (FBufferInfo*)_Overlapped;

	FChatPacket* packet = (FChatPacket*)Info->Buffer;

	delete Info;
}
