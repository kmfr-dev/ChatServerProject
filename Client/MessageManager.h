#pragma once

#include "../Common/DefineHeaders.h"
#include "Define.h"

class CMessageManager
{
public:
	CMessageManager() {};
	~CMessageManager() {};

public:
	bool Init();
	void End();
	
public:
	// 송/수신 쓰레드 초기화
	void InitRecvThread();
	void RecvChat();
	// 수신한 데이터면 전체 채팅 배열에 추가, 송신이면 비동기 송신
	void AddChat(EChatType _Type, const std::string& _Message, bool _IsSend);
	// 전체 채팅 메세지에서 오래된 메세지 삭제
	void EraseOldedChat();

private:
	// 비동기 수신 예약
	void RegisterRecv();
	// 송신 버퍼 동적할당 후 메인 쓰레드 -> 송/수신 쓰레드로 넘김
	void StartChatSend(EChatType _Type, const std::string& _Message);

private:
	static void RecvCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags);	
	static void RequestAsyncSend(ULONG_PTR _Ptr);
	static void SuccessAsyncSend(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags);

public:
	HANDLE GetThreadHandle() { return (HANDLE)mThread.native_handle(); }
	std::mutex& GetMutex() { return mChatMutex; }
	const std::deque<FChatData>& GetRecvChats() const { return mChats; }
	void ClearChatList() { mChats.clear(); }

private:
	// 서버에서 수신한 전체 채팅 데이터
	std::deque<FChatData> mChats;
	// 클라이언트가 서버로 송신할 패킷
	std::queue<FBufferInfo*> mSendQueue;

private:
	std::thread mThread;
	std::mutex mChatMutex;
};