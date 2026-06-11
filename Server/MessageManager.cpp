#include "MessageManager.h"

bool CMessageManager::Init()
{
	return true;
}

void CMessageManager::AddRecvChat(const std::string& _Message)
{
	if (mRecvChats.size() > SERVER_MAXLOGSIZE)
	{
		mRecvChats.clear();
	}

	mRecvChats.emplace_back(_Message);
}
