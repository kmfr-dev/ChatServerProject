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
	void SetName(const std::string& _NewName) { mName = _NewName; }
	void SetName(const char* _NewName) { mName = _NewName; }
	void SetServerIP(const char* _NewServerIP) { mServerIP = _NewServerIP; }
	void SetID(int _NewID) { mID = _NewID; }
	void SetConnected(bool _Connected) { mConnected.store(_Connected); }

public:
	SOCKET& GetSocket() { return mConnectedSocket; }
	std::mutex& GetMutex() { return mMutex; }
	FBufferInfo& GetRecvBuffer() { return mRecvBuffer; }

	const std::string& GetName() const { return mName; }
	const char* GetName_Cstr() const { return mName.c_str(); }
	const std::string& GetServerIP() const { return mServerIP; }
	const char* GetServerIP_Cstr() const { return mServerIP.c_str(); }
	int GetID() const { return mID; }
	bool IsConnected() const { return mConnected.load(); }

private:
	SOCKET mConnectedSocket = {};
	std::string mName = "";
	std::string mServerIP = "";
	int mID = -1;
	std::atomic<bool> mConnected = false;

private:
	FBufferInfo mRecvBuffer;
	std::mutex	mMutex;

	// 수신 패킷 조립 버퍼
	std::array<char, sizeof(FChatPacket)> mPacketBuffer{};
	size_t mPacketBytes = 0;

public:
	std::array<char, sizeof(FChatPacket)>& GetPacketBuffer() { return mPacketBuffer; }
	size_t& GetPacketBytes() { return mPacketBytes; }
};
