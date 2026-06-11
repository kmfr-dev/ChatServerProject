#pragma once

#include "../Common/DefineHeaders.h"
#include "Define.h"

class CClient
{
public:
	CClient() {};
	~CClient() {};

public:
	bool Init();
	void ConnectToServer();
	void End();

public:
	void SetName(const char* _NewName) { mName = _NewName; }
	void SetSerrverIP(const char* _NewServerIP) { mServerIP = _NewServerIP; }

public:
	SOCKET& GetSocket() { return mConnectedSocket; }
	std::mutex& GetMutex() { return mMutex; }
	FBufferInfo& GetRecvBuffer() { return mRecvBuffer; }

	const std::string& GetName() const { return mName; }
	const char* GetName_Cstr() const { return mName.c_str(); }
	const std::string& GetServerIP() const { return mServerIP; }
	const char* GetServerIP_Cstr() const { return mServerIP.c_str(); }

private:
	SOCKET mConnectedSocket = {};
	std::string mName = "";
	std::string mServerIP = "";

private:
	FBufferInfo mRecvBuffer;
	std::mutex	mMutex;

	// ============== SERVER LOAD TEST ==============
private:
	std::queue<FBufferInfo*> mSendQueue;
public:
	std::queue<FBufferInfo*>& GetSendQueue() { return mSendQueue; }
	// ==============================================
};