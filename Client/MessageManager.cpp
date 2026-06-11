#include "MessageManager.h"
#include "ChatServerApp.h"
#include "Client.h"
#include "TimerManager.h"

bool CMessageManager::Init()
{
	InitRecvThread();

	return true;
}

void CMessageManager::End()
{
	mRunning = false;

	if(mThread.joinable())
	{
		QueueUserAPC(&CMessageManager::EmptyFunc, (HANDLE)mThread.native_handle(), NULL);
		mThread.join();
	}
}

void CMessageManager::InitRecvThread()
{
	// 디버그일 경우에는 IOCP로 테스트
#ifdef _DEBUG
	SERVER_TEST_InitIOCP();
#else
	mRunning = true;
	mThread = std::thread(&CMessageManager::RecvChat, this);
#endif
}

void CMessageManager::RecvChat()
{
	// 송/수신 쓰레드가 처음에 수신등록.
	// Overlapped I/O 콜백 방식은 비동기 함수를 호출한 
	// 쓰레드가 대기 상태로 들어가야 콜백이 실행됨.
	RegisterRecv();

	while (mRunning)
	{
		// 콜백을 호출하려면
		// 이 쓰레드를 Alertable Wait 상태로 진입해야 함.
		// 데이터 송수신 완료가 될때 까지 쓰레드는 대기
		SleepEx(INFINITE, TRUE);
	}
}

void CMessageManager::AddChat(EChatType _Type, const std::string& _Message, bool _IsSend)
{
	if (_IsSend)
	{
		StartChatSend(_Type, _Message);
		return;
	}
	
	std::lock_guard<std::mutex> lock(mChatMutex);

	FChatData data(_Type, _Message);

	mChats.emplace_back(data);

	EraseOldedChat();
}

void CMessageManager::EraseOldedChat()
{
	// 채팅 배열이 최대 크기 보다 크다면 
	// 가장 예전 채팅 메세지를 제거
	if (mChats.size() > CLIENTMAX_CHATSIZE)
	{
		mChats.pop_front();
	}
}

void CMessageManager::RegisterRecv()
{
	CClient* client = CChatServerApp::GetInstance()->GetClient();
	if (nullptr == client)
	{
		std::cerr << "Client nullptr..!"  << std::endl;
		return;
	}


	std::lock_guard<std::mutex> lock(client->GetMutex());
	
	FBufferInfo& bufferInfo = client->GetRecvBuffer();
	ZeroMemory(&bufferInfo.Overlapped, sizeof(WSAOVERLAPPED));
	bufferInfo.rwMode = IO_MODE::READ;
	bufferInfo.WSABuf.buf = bufferInfo.Buffer;
	bufferInfo.WSABuf.len = sizeof(FChatPacket);
	
	DWORD flag = 0;

	// 비동기 수신 요청
	int result = WSARecv(client->GetSocket(), &bufferInfo.WSABuf, 1,
		nullptr, &flag, &bufferInfo.Overlapped, &RecvCallBack);

	if (SOCKET_ERROR == result)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			std::cerr << "WSARecv Error : " + std::to_string(error) << std::endl;
		}
	}
}

void CMessageManager::StartChatSend(EChatType _Type, const std::string& _Message)
{
	// 송신 버퍼 동적 할당, 완료되기 전에 스택에서 사라지기 때문에
	FBufferInfo* sendData = new FBufferInfo;
	sendData->rwMode = IO_MODE::WRITE;

	FChatPacket* packet = (FChatPacket*)sendData->Buffer;
	packet->Type = _Type;
	strncpy_s(packet->Message, _Message.c_str(), PACKET_SIZE);

	sendData->WSABuf.buf = sendData->Buffer;
	sendData->WSABuf.len = PACKET_SIZE;

	std::lock_guard<std::mutex> lock(mChatMutex);

	mSendQueue.push(sendData);

	// 실행흐름을 메인 -> 송수신 쓰레드로 넘김
	QueueUserAPC(&CMessageManager::RequestAsyncSend, (HANDLE)mThread.native_handle(), (ULONG_PTR)this);
}

void CMessageManager::EmptyFunc(ULONG_PTR _Ptr)
{
	std::cerr << "Client I/O Thread End!!" << std::endl;
}

void CALLBACK CMessageManager::RecvCallBack(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 버퍼 정보 구조체로 변환
	FBufferInfo* Info = (FBufferInfo*)_Overlapped;

	CMessageManager* messageManager = CChatServerApp::GetInstance()->GetMessageManager();

	// 에러 및 서버 종료라면
	if (nullptr == messageManager || _Error || _Bytes == 0)
	{
		CChatServerApp::GetInstance()->GetMessageManager()->
			AddChat(EChatType::CHAT_TYPE_ERROR, "서버와 연결이 끊겼습니다..", false);
		return;
	}

	// 수신된 패킷을 처리
	FChatPacket* packet = (FChatPacket*)Info->Buffer;
	messageManager->AddChat(packet->Type, packet->Message, false);

	// 클라이언트는 다음 데이터를 받기 위해 수신 등록
	messageManager->RegisterRecv();
}

void CALLBACK CMessageManager::RequestAsyncSend(ULONG_PTR _Ptr)
{
	CMessageManager* messageManager = (CMessageManager*)_Ptr;
	
	// 락 잡기
	std::lock_guard<std::mutex> lock(messageManager->mChatMutex);

	std::queue<FBufferInfo*>& sendQueue = messageManager->mSendQueue;

	while (!sendQueue.empty())
	{
		FBufferInfo* sendData = sendQueue.front();
		sendQueue.pop();

		DWORD sendBytes = 0;
		SOCKET clientSock = CChatServerApp::GetInstance()->GetClient()->GetSocket();

		// 실제로 비동기 수신 요청.
		WSASend(clientSock, &sendData->WSABuf, 1, &sendBytes, 0, &sendData->Overlapped, &CMessageManager::SuccessAsyncSend);
	}
}

void CMessageManager::SuccessAsyncSend(DWORD _Error, DWORD _Bytes, LPWSAOVERLAPPED _Overlapped, DWORD _Flags)
{
	// 송신 콜백이면, 
	// 이전에 동적할당된 버퍼 정보이므로 메모리만 해제한다.
	FBufferInfo* bufferInfo = (FBufferInfo*)_Overlapped;

	delete bufferInfo;
}

// SERVER LOAD TEST

void CMessageManager::SERVER_TEST_PROCESSIO()
{
	while (true)
	{
		DWORD		byetsRansferred = 0;
		CClient* client = nullptr;
		WSAOVERLAPPED* overlapped = nullptr;

		BOOL result = GetQueuedCompletionStatus(mhIOCP, &byetsRansferred, (PULONG_PTR)&client, &overlapped, INFINITE);
		// 종료 조건일 경우
		if (result == TRUE && client == nullptr && overlapped == nullptr)
			break;

		if (FALSE == result)
			continue;

		FBufferInfo* IOData = (FBufferInfo*)overlapped;
		if (IOData->rwMode == IO_MODE::WRITE)
		{
			delete IOData;

			if (!mRunning)
				break;

			continue;
		}

		// 읽기 모드면, Client -> Server로 전송한 정보
		if (IOData->rwMode == IO_MODE::READ)
		{
			// 수신 예약
			SERVER_TEST_RegisterRecv(client);
		}

	}
}

void CMessageManager::SERVER_TEST_SendToServer()
{
	while (mRunning)
	{
		float DeltaSeconds = CTimerManager::GetInstance()->UpdateTick();
		mTimes += DeltaSeconds;

		if (mTimes >= SERVERTEST_CLIENT_SENDINTERVAL)
		{
			std::lock_guard<std::mutex> lock(CChatServerApp::GetInstance()->GetMutex());
			const std::vector<CClient*>& Clients = CChatServerApp::GetInstance()->GetTestClients();

			for (int i = 0; i < Clients.size(); ++i)
			{
				CClient* DummyClient = Clients[i];
				if (nullptr == DummyClient)
					continue;

				SERVER_TEST_StartChatSend(DummyClient, DummyClient->GetName());
			}

			mTimes = 0.0f;
		}
	}
}

void CMessageManager::SERVER_TEST_InitIOCP()
{
	mhIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, mThreadCount);
	if (NULL == mhIOCP)
	{
		std::cerr << "CP object create failed. error code: " << GetLastError() << std::endl;
		return;
	}
	
	WSADATA wsa = {};

	int wsaStartResult = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (0 != wsaStartResult)
	{
		std::cerr << "WSAStartup failed.. error code : " << wsaStartResult << std::endl;
		return;
	}

	const std::vector<CClient*> TestClients = CChatServerApp::GetInstance()->GetTestClients();
	for (int i = 0; i < TestClients.size(); ++i)
	{
		CClient* DummyClient = TestClients[i];

		if (nullptr == DummyClient)
			continue;

		SOCKET& Sock = DummyClient->GetSocket();
		Sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		
		if (INVALID_SOCKET == Sock)
			continue;

		SOCKADDR_IN addr = { };
		addr.sin_family = AF_INET;
		addr.sin_port = htons(PORT);
		inet_pton(AF_INET, DummyClient->GetServerIP_Cstr(), &addr.sin_addr.s_addr);

		if (connect(Sock, (SOCKADDR*)&addr, sizeof(addr)))
		{
			std::cerr << "Connected failed.." << std::endl;
			continue;
		}

		if (INVALID_SOCKET == Sock)
			continue;

		CreateIoCompletionPort((HANDLE)Sock, mhIOCP, (ULONG_PTR)DummyClient, 0);

		DummyClient->GetRecvBuffer().rwMode = IO_MODE::READ;

		SERVER_TEST_RegisterRecv(DummyClient);
	}

	mThreadCount = std::thread::hardware_concurrency() * 2;

	mWorkerThreads.reserve(mThreadCount);
	for (int i = 0; i < mThreadCount; ++i)
	{
		mWorkerThreads.emplace_back(std::thread(&CMessageManager::SERVER_TEST_PROCESSIO, this));
	}

	// 틱쓰레드 돌리기
	mTickThread = std::thread(&CMessageManager::SERVER_TEST_SendToServer, this);
	mRunning = true;
}

void CMessageManager::SERVER_TEST_RegisterRecv(CClient* _Client)
{
	if (nullptr == _Client)
	{
		std::cerr << "Client nullptr..!" << std::endl;
		return;
	}

	// 특정 클라 락잡기
	std::lock_guard<std::mutex> lock(_Client->GetMutex());

	FBufferInfo& bufferInfo = _Client->GetRecvBuffer();
	ZeroMemory(&bufferInfo.Overlapped, sizeof(WSAOVERLAPPED));

	bufferInfo.rwMode = IO_MODE::READ;
	bufferInfo.WSABuf.buf = bufferInfo.Buffer;
	bufferInfo.WSABuf.len = sizeof(FChatPacket);

	DWORD flag = 0;

	// 비동기 수신 요청
	int result = WSARecv(_Client->GetSocket(), &bufferInfo.WSABuf, 1,
		nullptr, &flag, &bufferInfo.Overlapped, nullptr);

	if (SOCKET_ERROR == result)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			std::cerr << "WSARecv Error : " + std::to_string(error) << std::endl;
		}
	}
}

void CMessageManager::SERVER_TEST_StartChatSend(CClient* _Client, const std::string& _Message)
{
	if (nullptr == _Client)
	{
		std::cerr << "SERVER_TEST_StartChatSend() : Client nullptr!" << std::endl;
		return;
	}

	// 송신 버퍼 동적 할당, 완료되기 전에 스택에서 사라지기 때문에
	FBufferInfo* sendData = new FBufferInfo;
	sendData->WSABuf.buf = sendData->Buffer;
	sendData->WSABuf.len = PACKET_SIZE;
	sendData->rwMode = IO_MODE::WRITE;

	FChatPacket* packet = (FChatPacket*)sendData->Buffer;
	packet->Type = EChatType::CHAT_TYPE_NORMAL;
	strncpy_s(packet->Message, _Message.c_str(), PACKET_SIZE);

	std::lock_guard<std::mutex> lock(_Client->GetMutex());
	_Client->GetSendQueue().push(sendData);

	DWORD sendBytes = 0;

	// 비동기 송신 함수 호출
	if (SOCKET_ERROR == WSASend(_Client->GetSocket(), &sendData->WSABuf, 1, &sendBytes,
		0, &sendData->Overlapped, nullptr))
	{
		// 에러 발생시 위에서 동적할당한 데이터 메모리 헤제
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			delete sendData;
		}
	}
}