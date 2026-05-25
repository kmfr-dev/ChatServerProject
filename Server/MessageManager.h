#pragma once

#include "DefineHeaders.h"

class CMessageManager
{
public:
	CMessageManager() {};
	~CMessageManager() {};

public:
	bool Init();
	const std::vector<std::string>& GetRecvChats() const { return mRecvChats; }	
	void ClearChatList() { mRecvChats.clear(); }
	void AddRecvChat(const std::string& _Message) { mRecvChats.push_back(_Message); };

private:
	std::vector<std::string> mRecvChats;
};

