#pragma once

#include "resource.h"
#include "Define.h"
#include "DefineHeaders.h"

class CServer
{
	friend class CChatServerApp;

private:
	CServer() {};
	~CServer() {};

public:
	int		Start();
	void	End();

private:
	void InitThread();

private:
	void AcceptClient();
	void RecvChat();

private:
	SOCKET mListeningSocket = {};
	HANDLE mhIOCP = {};

	int mThreadCount = 0;
	std::thread mAcceptThread;
	std::vector<std::thread> mWorkerThreads;
};