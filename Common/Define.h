#pragma once

#include "Enum.h"
#include "DefineHeaders.h"

#define MAX_LOADSTRING 100
#define PORT 9190
#define BUF_SIZE 4096
#define PACKET_SIZE 1024
#define MAX_CHATSIZE 20
#define SERVER_MAXLOGSIZE 5000

struct FBufferInfo
{
	// Buffer Info
	WSAOVERLAPPED Overlapped = {};
	WSABUF WSABuf = {};
	char Buffer[BUF_SIZE] = {};
	IO_MODE rwMode = IO_MODE::NONE;
};

struct FChatPacket
{
	EChatType Type = EChatType::CHAT_TYPE_NONE;
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