#pragma once

#include "Enum.h"
#include "DefineHeaders.h"

#define MAX_LOADSTRING 100
#define PORT 9190
#define BUF_SIZE 4096
#define PACKET_SIZE 1024
#define CLIENTMAX_CHATSIZE 20
#define MAX_CLIENT 500
#define SERVER_MAXLOGSIZE 5000
#define SERVER_IP "127.0.0.1"
#define SERVER_LOADTEST_SEND_INTERVAL 1.0f
#define POOL_SIZE 2000
#define MEMORY_CHUNKSIZE 1024
#define MAX_MEMORY_CHUNKS 64
#define MAX_PENDING_SEND_PER_CLIENT 256

struct FBufferInfo
{
	// Buffer Info
	WSAOVERLAPPED Overlapped = {};
	WSABUF WSABuf = {};
	char Buffer[BUF_SIZE] = {};
	IO_MODE rwMode = IO_MODE::NONE;
	size_t TotalBytes = 0;
	size_t TransferredBytes = 0;
};

struct FChatPacket
{
	EChatType Type = EChatType::CHAT_TYPE_NONE;
	int SenderID = -1;
	long long TimeStamp = 0;
	char Message[PACKET_SIZE] = { 0, };
};

struct FChatData
{
	FChatData(EChatType _Type, const std::string& _Message)
		: ChatType(_Type), Message(_Message)
	{

	}

	~FChatData() {};

	EChatType ChatType = EChatType::CHAT_TYPE_NONE;
	std::string Message = "";
};

struct FRTTStats
{
	long long TotalRTT = 0;
	long long ResponseCnt = 0;
};

class CUtils
{
public:
	static FChatPacket MakePacket(EChatType _Type, const std::string& _Message, 
		int _SenderID = -1, long long _TimeStamp = 0)
	{
		FChatPacket ReturnPacket{};
		ReturnPacket.Type = _Type;
		ReturnPacket.SenderID = _SenderID;
		ReturnPacket.TimeStamp = _TimeStamp;

		return ReturnPacket;
	}
};