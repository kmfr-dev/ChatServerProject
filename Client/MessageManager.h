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
	void InitRecvThread();
	void RecvChat();
	void AddChat(EChatType _Type, const std::string& _Message, bool _IsSend);
	void EraseOldedChat();

public:
	static void RecvCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags);
	static void SendCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags);

public:
	std::mutex& GetMutex() { return mChatMutex; }
	const std::vector<FChatData>& GetRecvChats() const { return mChats; }
	void ClearChatList() { mChats.clear(); }

private:
	// 실제 채팅 데이터
	std::vector<FChatData> mChats;

private:
	std::thread mThread;
	std::mutex mChatMutex;
};