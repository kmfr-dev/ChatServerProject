#pragma once

#include "resource.h"
#include "../Common/Define.h"
#include "../Common/DefineHeaders.h"

struct FClient
{
	// 클라이언트의 ID
	int ID = -1;

	// Socket Info
	SOCKET ClientSock = {};
	SOCKADDR_IN Addr = {};
	
	int AddrSize = sizeof(Addr);

	// Buffer Info
	FBufferInfo BufferInfo;

	// 패킷 조립을 위한 Buffer를 각 클라마다 할당
	std::array<char, sizeof(FChatPacket)> PacketBuffer{ };
	size_t PacketBytes = 0;

	std::atomic<bool> IsClosing = false;
	std::atomic<long long> PendingSendCount = 0;
	std::mutex SendMutex;
	
	// Batching 작업
	// SendQueue : 아직 송신 배치에 포함되지 않은 논리 패킷
	std::deque<FChatPacket> SendQueue;
	// 이 클라이언트 소켓에 WSASend가 진행 중인지에 대한 여부
	bool SendInProgress = false;
};

class CServer
{
public:
	CServer() {};
	~CServer() {};

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
	std::vector<std::pair<FClient*, std::string>> mClients;
	std::vector<FClient*> mClosingClients;
	// 나간 클라들 ID 재사용을 위한 set
	std::set<int> mDeathIDs;

public:
	int		Start();
	void	End();

private:
	// 워커 쓰레드 Init
	void InitThread();
	// 인자로 넘어온 클라 송/수신 등록 
	bool RegisterRecv(FClient* _Client);
	bool RegisterSend(FClient* _Client, FBufferInfo* _SendData);

	// 메시지 Broadcast
	void BroadCastPacket(const FChatPacket& _Packet);
	// 메시지를 메시지 매니저에 저장
	void AddChatMsg(const std::string& _Message);

private:
	// 특정 클라 메모리 해제 / 메시지 브로드캐스트
	void ExitClient(FClient* _Client);
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
	// TCP로 받은 Byte를 Client별 조립 버퍼에 누적, 
	// 하나의 패킷 만큼 모이면 완성된 패킷으로 처리
	void AppendRecvData(FClient* _Client, const char* _Data, size_t _RecvBytes);
	void ProcessPacket(FClient* _Client, const FChatPacket& _Packet);

private:
	bool EnqueueSend(FClient* _Client, const FChatPacket& _Packet);
	void PostNextBatch(FClient* _Client);
	void CompleteSend(FClient* _Client, FBufferInfo* _SendContext);
	void FailSend(FClient* _Client, FBufferInfo* _SendContext);


	// Load Test
private:
	std::atomic<long long> mQueuedPacketCount = 0;
	std::atomic<long long> mPendingSendCount = 0;
	std::atomic<long long> mDroppedSendCount = 0;
	std::atomic<long long> mConnectedClientCount = 0;

public:
	long long GetQueuedPacketCount() const { return mQueuedPacketCount.load(); }
	long long GetPendingSendCount() const { return mPendingSendCount.load(); }
	long long GetDroppedSendCount() const { return mDroppedSendCount.load(); }
	long long GetConnectedClientCount() const { return mConnectedClientCount.load(); }
};
