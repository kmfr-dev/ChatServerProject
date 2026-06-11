#pragma once

#include "../Common/DefineHeaders.h"
#include "../Common/Define.h"

class CMessageManager
{
public:
	CMessageManager() {};
	~CMessageManager() {};

public:
	bool Init();
	const std::deque<std::string>& GetRecvChats() const { return mRecvChats; }
	void ClearChatList() { mRecvChats.clear(); }
	void AddRecvChat(const std::string& _Message);

private:
	std::deque<std::string> mRecvChats;
};

