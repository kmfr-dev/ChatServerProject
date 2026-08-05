#pragma once

#include "Enum.h"
#include "DefineHeaders.h"

#define MAX_LOADSTRING 100
#define PORT 9190
#define BUF_SIZE 4096
#define PACKET_SIZE 1024
#define CLIENTMAX_CHATSIZE 20
#define MAX_CLIENT 1000

#define SERVER_MAXLOGSIZE 5000
#define SERVER_IP "127.0.0.1"

#define SERVER_LOADTEST_SEND_INTERVAL 1.0f
#define POOL_SIZE 2000
#define MEMORY_CHUNKSIZE 1024
#define MAX_MEMORY_CHUNKS 64

#define MAX_QUEUED_PACKET_PER_CLIENT 20
#define MAX_BATCH_PACKET_COUNT 3

struct FBufferInfo
{
	// Buffer Info
	WSAOVERLAPPED Overlapped = {};
	WSABUF WSABuf = {};
	char Buffer[BUF_SIZE] = {};
	IO_MODE rwMode = IO_MODE::NONE;
	size_t TotalBytes = 0;
	size_t TransferredBytes = 0;

	// 송신 배치에 들어간 FChatPacket 개수
	size_t PacketCount = 0;
};

struct FChatPacket
{
	EChatType Type = EChatType::CHAT_TYPE_NONE;
	int SenderID = -1;
	long long TimeStamp = 0;
	char Message[PACKET_SIZE] = { 0, };
};

static_assert(sizeof(FChatPacket) * MAX_BATCH_PACKET_COUNT <= BUF_SIZE, 
	"Batch buffer size exceeded.");

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
		strncpy_s(ReturnPacket.Message, _Message.c_str(), PACKET_SIZE);

		return ReturnPacket;
	}
};
