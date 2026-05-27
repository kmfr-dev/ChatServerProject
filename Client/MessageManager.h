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
	const std::vector<FChatData>& GetRecvChats() const { return mChats; }
	void ClearChatList() { mChats.clear(); }

private:
	std::vector<FChatData> mChats;

private:
	std::thread mThread;
	std::mutex mChatMutex;
};