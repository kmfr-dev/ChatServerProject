#pragma once

#include "resource.h"
#include "Define.h"
#include "DefineHeaders.h"

struct CClient
{
	int ID = -1;

	// Socket Info
	SOCKET ClientSock = {};
	SOCKADDR_IN Addr = {};
	
	int AddrSize = sizeof(Addr);

	// Buffer Info
	OVERLAPPED Overlapped = {};
	WSABUF WSABuf = {};
	char Buffer[BUF_SIZE] = {};
	IO_MODE rwMode = IO_MODE::NONE;
};

class CServer
{
public:
	CServer() {};
	~CServer() {};

public:
	int		Start();
	void	End();

private:
	void InitThread();
	void RegisterRecv(CClient* _Client);
	void BroadCastMsg(const std::string& _Message);
	void AddChatMsg(const std::string& _Message);

private:
	void ExitClient(CClient* _Client);
	void ShutdownAllClient();
	void ClearClient();

private:
	// Thread Func
	void AcceptClient();
	void RecvChat();

private:
	SOCKET mListeningSocket = {};
	HANDLE mhIOCP = {};

private:
	int mThreadCount = 0;
	std::atomic<bool> mRunning = false;

	std::thread mAcceptThread;
	std::vector<std::thread> mWorkerThreads;
	std::mutex mClientMutex;

private:
	std::vector<std::pair<CClient*, std::string>> mClients;
	std::set<int> mDeathIDs;
};