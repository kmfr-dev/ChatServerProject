#pragma once

#include "resource.h"
#include "../Common/Define.h"
#include "../Common/DefineHeaders.h"

struct FBufferInfo
{
	// Buffer Info
	OVERLAPPED Overlapped = {};
	WSABUF WSABuf = {};
	char Buffer[BUF_SIZE] = {};
	IO_MODE rwMode = IO_MODE::NONE;
};

struct CClient
{
	// 클라이언트의 ID
	int ID = -1;

	// Socket Info
	SOCKET ClientSock = {};
	SOCKADDR_IN Addr = {};
	
	int AddrSize = sizeof(Addr);

	// Buffer Info
	FBufferInfo BufferInfo;
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
	// 워커 쓰레드 Init
	void InitThread();
	// 인자로 넘어온 클라 수신 등록 
	void RegisterRecv(CClient* _Client);
	// 메시지 Broadcast
	void BroadCastMsg(EChatType _Type, const std::string& _Message);
	// 메시지를 메시지 매니저에 저장
	void AddChatMsg(const std::string& _Message);

private:
	// 특정 클라 메모리 해제 / 메시지 브로드캐스트
	void ExitClient(CClient* _Client);
	// 모든 클라이언트의 소켓 shutdown
	void ShutdownAllClient();
	// 모든 클라이언트 소켓, 메모리 해제
	void ClearClient();

private:
	// Thread Func
	// 연결 요청 쓰레드 함수
	void AcceptClient();
	// I/O 처리 워커 쓰레드 함수
	void ProcessIO();

private:
	// 서버소켓
	SOCKET mListeningSocket = {};
	// IOCP 핸들
	HANDLE mhIOCP = {};

private:
	// 쓰레드 개수
	int mThreadCount = 0;
	// 연결요청 수락 쓰레드의 루프 flag
	std::atomic<bool> mRunning = false;

	// 연결 요청을 위한 쓰레드 - 1개
	std::thread mAcceptThread;
	// I/O 처리를 위한 쓰레드 - 다수
	std::vector<std::thread> mWorkerThreads;
	// 동기화 처리를 위한 뮤텍스
	std::mutex mClientMutex;

private:
	// 클라이언트 배열 pair : <클라, 클라이름>
	std::vector<std::pair<CClient*, std::string>> mClients;
	// 나간 클라들 ID 재사용을 위한 set
	std::set<int> mDeathIDs;
};