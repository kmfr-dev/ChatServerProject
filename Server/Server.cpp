#include "Server.h"
#include "ChatServerApp.h"
#include "MessageManager.h"
#include "MemoryPoolManager.h"

int CServer::Start()
{
	WSADATA wsaData = {};

	// Winsock 초기화
	// 초기화 실패시 0이 아닌 값이 반환됨
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (0 != wsaResult)
	{
		std::cerr << "WSAStartup failed. error code : " << wsaResult << std::endl;
		return 1;
	}

	// 소켓 생성
	// IPv4, 연결지향형 소켓, TCP 프로토콜 사용
	mListeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == mListeningSocket)
	{
		std::cerr << "Listening socket, Invalid Socket : " << mListeningSocket << std::endl;
		return 1;
	}

	// 소켓에 IP/PORT번호 할당
	// INADDR_ANY : 어떤 IP로 들어오던 다 받겠다는 의미, 
	// 특정 IP를 명시하면 그 IP로 들어오는 연결만 수락한다.

	SOCKADDR_IN serv_Addr = { 0 };
	serv_Addr.sin_family = AF_INET;
	serv_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_Addr.sin_port = htons(PORT);

	int bindResult = bind(mListeningSocket, (sockaddr*)&serv_Addr, sizeof(serv_Addr));

	if (0 != bindResult)
	{
		std::cerr << "Bind failed. error code: " << bindResult << std::endl;
		return 1;
	}

	// 소켓을 연결요청을 받을 수 있는 상태로 전환 (리스닝 소켓으로 전환)
	int listenResult = listen(mListeningSocket, SOMAXCONN);

	if (0 != listenResult)
	{
		std::cerr << "Listen failed. error code: " << listenResult << std::endl;
		return 1;
	}

	// 이 컴퓨터 CPU의 논리코어수를 얻어온다.
	mThreadCount = std::thread::hardware_concurrency();

	// CP 오브젝트 생성, I/O를 처리할 쓰레드 갯수 할당
	mhIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, mThreadCount);
	if (NULL == mhIOCP)
	{
		std::cerr << "CP object create failed. error code: " << GetLastError() << std::endl;
		return 1;
	}
	
	// 클라이언트 연결 요청을 수락할 쓰레드 생성
	mRunning = true;
	mAcceptThread = std::thread(&CServer::AcceptClient, this);

	// I/O처리를 담당할 쓰레드 생성
	InitThread();

	return 0;
}

void CServer::End()
{
	{
		std::lock_guard<std::mutex> lock(mClientMutex);
		mRunning = false;
	}
	
	closesocket(mListeningSocket);

	// 모든 클라이언트 소켓 shutdown 
	ShutdownAllClient();

	// 현재 I/O를 처리하는 IOCP 완료 큐에 스레드 수만큼 종료 신호를 보냄
	// 그러면 I/O를 처리하는 쓰레드가 큐에서 종료 신호를 꺼내감
	for (int i = 0; i < mWorkerThreads.size(); ++i)
	{
		PostQueuedCompletionStatus(mhIOCP, 0, 0, nullptr);
	}

	// 만약 쓰레드가 종료가 안된상태로 
	// IOCP 핸들 및 소켓을 닫아버리면 리소스가 다 날아가버릴 수 있음
	mAcceptThread.join();
	for (std::thread& thread : mWorkerThreads)
		thread.join();
	
	// 클라이언트 메모리 정리
	ClearClient();

	mClients.clear();
	mClosingClients.clear();
	mDeathIDs.clear();

	CloseHandle(mhIOCP);
	WSACleanup();
}

void CServer::InitThread()
{
	mWorkerThreads.reserve(mThreadCount);

	for (int i = 0; i < mThreadCount; ++i)
	{
		mWorkerThreads.emplace_back(std::thread(&CServer::ProcessIO, this));
	}
}

bool CServer::RegisterRecv(FClient* _Client)
{
	if (nullptr == _Client)
		return false;

	ZeroMemory(&_Client->BufferInfo.Overlapped, sizeof(WSAOVERLAPPED));

	_Client->BufferInfo.rwMode = IO_MODE::READ;
	_Client->BufferInfo.WSABuf.buf = _Client->BufferInfo.Buffer;
	_Client->BufferInfo.WSABuf.len = BUF_SIZE;

	DWORD flags = 0;
	const int result = WSARecv(_Client->ClientSock, &_Client->BufferInfo.WSABuf, 1,
		nullptr, &flags, &_Client->BufferInfo.Overlapped, nullptr);

	if (SOCKET_ERROR == result && WSAGetLastError() != WSA_IO_PENDING)
		return false;

	return true;
}

void CServer::BroadCastPacket(const FChatPacket& _Packet)
{
	std::lock_guard<std::mutex> lock(mClientMutex);

	// 전체 클라이언트에게 브로드 캐스트
	for (std::pair<FClient*, std::string>& pair : mClients)
	{
		FClient* client = pair.first;

		if (nullptr == client || INVALID_SOCKET == client->ClientSock || client->IsClosing)
			continue;

		if (client->PendingSendCount.load() >= MAX_PENDING_SEND_PER_CLIENT)
		{
			mDroppedSendCount.fetch_add(1);
			continue;
		}

		// 송신용 데이터 동적할당, 수신 완료 후 워커 쓰레드에서 해제함.
		FBufferInfo* sendData = CMemoryPoolManager::GetInstance()->Get();
		if (nullptr == sendData)
		{
			mDroppedSendCount.fetch_add(1);
			continue;
		}

		sendData->rwMode = IO_MODE::WRITE;
		sendData->TotalBytes = sizeof(_Packet);
		sendData->TransferredBytes = 0;
		memcpy(sendData->Buffer, &_Packet, sizeof(_Packet));
		sendData->WSABuf.buf = sendData->Buffer;
		sendData->WSABuf.len = static_cast<ULONG>(sendData->TotalBytes);

		if (!RegisterSend(client, sendData))
		{
			CMemoryPoolManager::GetInstance()->Release(sendData);
			mDroppedSendCount.fetch_add(1);
		}
	}
}

bool CServer::RegisterSend(FClient* _Client, FBufferInfo* _SendData)
{
	if (!_Client || !_SendData || INVALID_SOCKET == _Client->ClientSock)
		return false;

	_Client->PendingSendCount.fetch_add(1);
	mPendingSendCount.fetch_add(1);

	DWORD sendBytes = 0;
	const int result = WSASend(_Client->ClientSock, &_SendData->WSABuf, 1,
		&sendBytes, 0, &_SendData->Overlapped, nullptr);

	if (SOCKET_ERROR == result && WSAGetLastError() != WSA_IO_PENDING)
	{
		_Client->PendingSendCount.fetch_sub(1);
		mPendingSendCount.fetch_sub(1);
		return false;
	}

	return true;
}

void CServer::AddChatMsg(const std::string& _Message)
{
	std::lock_guard<std::mutex> lock(mClientMutex);

	// 메시지 매니저 배열에 메세지 추가
	CMessageManager* MsgManager = CChatServerApp::GetInstance()->GetMessageManager();
	if (nullptr != MsgManager)
	{
		MsgManager->AddRecvChat(_Message);
	}
}

void CServer::ExitClient(FClient* _Client)
{
	if (nullptr == _Client)
		return;

	bool Expected = false;
	if (!_Client->IsClosing.compare_exchange_strong(Expected, true))
		return;

	std::string clientName;

	{
		std::lock_guard<std::mutex> lock(mClientMutex);
		int ID = _Client->ID;
		// 클라 이름 복사 해두기
		clientName = mClients[ID].second;

		// 클라 소켓 닫기
		closesocket(_Client->ClientSock);
		_Client->ClientSock = INVALID_SOCKET;
	
		mClients[ID].second = "";
		mClients[ID].first = nullptr;
		mDeathIDs.insert(ID);

		mClosingClients.emplace_back(_Client);
		mConnectedClientCount.fetch_sub(1);
	}

	// 나간 클라이언트 메시지 브로드 캐스트
	if (!clientName.empty())
	{
		std::string exitMsg = "[" + clientName + "] 님이 나가셨습니다.";
		AddChatMsg(exitMsg);   

		BroadCastPacket(CUtils::MakePacket(EChatType::CHAT_TYPE_EXIT, exitMsg));
	}
}

void CServer::ShutdownAllClient()
{
	std::lock_guard<std::mutex> lock(mClientMutex);

	// 모든 클라 순회 - 소켓의 우아한 종료 
	for (const std::pair<FClient*, std::string>& pair : mClients)
	{
		if (nullptr == pair.first)
			continue;

		if (INVALID_SOCKET == pair.first->ClientSock)
			continue;

		shutdown(pair.first->ClientSock, SD_BOTH);
	}
}

void CServer::ClearClient()
{
	std::lock_guard<std::mutex> lock(mClientMutex);

	// 모든 클라 순회 - 클라 소켓 닫기 및 메모리 해제
	for (std::pair<FClient*, std::string>& pair : mClients)
	{
		if (nullptr == pair.first)
			continue;

		closesocket(pair.first->ClientSock);
		
		delete pair.first;
		pair.first = nullptr;
	}

	for (FClient*& client : mClosingClients)
	{
		delete client;
		client = nullptr;
	}
	mClosingClients.clear();
}

void CServer::AcceptClient()
{
	while (mRunning)
	{
		// 클라 생성 후 연결 요청 수락
		FClient* NewClient = new FClient;
		NewClient->ClientSock = accept(mListeningSocket, (sockaddr*)&NewClient->Addr, &NewClient->AddrSize);

		if (INVALID_SOCKET == NewClient->ClientSock)
		{
			delete NewClient;
			continue;
		}
		
		int ID = 0;

		// 임계구역 들어가기전에 Lock
		std::lock_guard<std::mutex> lock(mClientMutex);

		// 이미 나간 클라이언트의 ID가 있다면 그 ID 재사용
		if (mDeathIDs.size() > 0)
		{

			ID = *(mDeathIDs.begin());
			mDeathIDs.erase(mDeathIDs.begin());

			if (nullptr != mClients[ID].first)
			{
				delete mClients[ID].first;
				mClients[ID].first = nullptr;
			}

			mClients[ID].first = NewClient;
		}
		
		// 없다면 새로 ID 할당후 클라이언트 배열에 추가
		else
		{
			ID = (int)mClients.size();
			
			mClients.push_back(std::make_pair(NewClient, ""));
		}

		NewClient->ID = ID;

		// CP 오브젝트에 클라이언트 소켓 등록
		HANDLE portResult = CreateIoCompletionPort(
			(HANDLE)NewClient->ClientSock, mhIOCP, (ULONG_PTR)NewClient, 0);

		// Recv 등록 -> 데이터가 오면 IOCP가 워커 쓰레드를 깨워줌
		if (NULL == portResult || !RegisterRecv(NewClient))
		{
			closesocket(NewClient->ClientSock);
			mClients[ID].first = nullptr;
			mClients[ID].second.clear();
			mDeathIDs.insert(ID);
			delete NewClient;
			continue;
		}

		mConnectedClientCount.fetch_add(1);
	}
}

void CServer::ProcessIO()
{
	while (true)
	{
		DWORD		bytesRansferred = 0;
		FClient*	client = nullptr;
		WSAOVERLAPPED* overlapped = nullptr;

		// IOCP 완료큐에서 이벤트를 꺼낸다. 올 때까지 블로킹.
		BOOL result = GetQueuedCompletionStatus(mhIOCP, &bytesRansferred, (PULONG_PTR)&client, &overlapped, INFINITE);

		// 종료 조건일 경우
		if (result == TRUE && client == nullptr && overlapped == nullptr)
			break;

		// I/O 완료 정보를 얻어옴
		FBufferInfo* IOData = (FBufferInfo*)overlapped;

		// 실패했다면
		if (FALSE == result)
		{
			if (IOData && IOData->rwMode == IO_MODE::WRITE)
			{
				if (client)
					client->PendingSendCount.fetch_sub(1);
				mPendingSendCount.fetch_sub(1);
				CMemoryPoolManager::GetInstance()->Release(IOData);
			}
			
			if(client)
				ExitClient(client);

			continue;
		}

		// 클라가 정상 종료 했다면
		if (0 == bytesRansferred)
		{
			if (IOData && IOData->rwMode == IO_MODE::WRITE)
			{
				if (client)
					client->PendingSendCount.fetch_sub(1);
				mPendingSendCount.fetch_sub(1);
				CMemoryPoolManager::GetInstance()->Release(IOData);
			}

			ExitClient(client);

			continue;
		}

		// 만약 쓰기모드면, Server -> Client로 전송한 것이므로 
		// 동적할당한 송신데이터를 해제.
		if (IOData->rwMode == IO_MODE::WRITE)
		{
			// 받은 데이터 크기만큼 누적
			IOData->TransferredBytes += bytesRansferred;

			// 패킷 하나의 크기
			if (IOData->TransferredBytes < IOData->TotalBytes)
			{
				ZeroMemory(&IOData->Overlapped, sizeof(IOData->Overlapped));
				IOData->WSABuf.buf = IOData->Buffer + IOData->TransferredBytes;
				IOData->WSABuf.len = static_cast<ULONG>(
					IOData->TotalBytes - IOData->TransferredBytes);

				DWORD sendBytes = 0;
				const int sendResult = WSASend(client->ClientSock, &IOData->WSABuf, 1,
					&sendBytes, 0, &IOData->Overlapped, nullptr);

				if (SOCKET_ERROR == sendResult && WSAGetLastError() != WSA_IO_PENDING)
				{
					client->PendingSendCount.fetch_sub(1);
					mPendingSendCount.fetch_sub(1);
					mDroppedSendCount.fetch_add(1);
					CMemoryPoolManager::GetInstance()->Release(IOData);
					ExitClient(client);
				}

				continue;
			}

			if (client)
				client->PendingSendCount.fetch_sub(1);
			mPendingSendCount.fetch_sub(1);
			CMemoryPoolManager::GetInstance()->Release(IOData);

			continue;
		}

		// 읽기 모드면, Client -> Server로 전송한 정보
		else if (IOData->rwMode == IO_MODE::READ)
		{
			AppendRecvData(client, IOData->Buffer, bytesRansferred);
	
			// 다음 수신을 예약
			if (!RegisterRecv(client))
				ExitClient(client);
		}
	}
}

void CServer::AppendRecvData(FClient* _Client, const char* _Data, size_t _RecvBytes)
{
	// WSARecv로 받은 데이터에서 아직 처리하지 않은 위치값
	size_t offset = 0;
	
	// 한 번의 수신에 여러 패킷이 포함될 수 있으므로, 
	// 수신 데이터를 전부 소비할 떄 까지 반복
	while (offset < _RecvBytes)
	{
		// 현재 조립 중인 패킷을 완성하기 위해 추가로 필요한 Byte 수를 계산
		const size_t required = sizeof(FChatPacket) - _Client->PacketBytes;
		// 복사할 패킷 크기 계산 둘 중 작은 값만 복사해야 버퍼를 넘지 않음. 
		// 1. 패킷 완성에 필요한 바이트, 2. 현재 Recv 데이터에 남은 바이트
		const size_t copyBytes = (std::min)(required, _RecvBytes - offset);

		// Recv 버퍼의 데이터를 클라이언트 별 패킷 조립 버퍼에 누적
		std::memcpy(_Client->PacketBuffer.data() + _Client->PacketBytes,
				_Data + offset, copyBytes);

		// 조립 버퍼에 저장된 데이터 크기 갱신
		_Client->PacketBytes += copyBytes;
		// 이번 Recv 데이터에서 소비한 위치 갱신
		offset += copyBytes;

		// 고정 크기 패킷 하나가 완성됐는지 확인
		if (_Client->PacketBytes == sizeof(FChatPacket))
		{
			FChatPacket packet{};
			// 조립 버퍼를 실제 패킷 객체로 복사
			std::memcpy(&packet, _Client->PacketBuffer.data(), sizeof(packet));

			// 패킷 처리 완료됐으므로 다음 패킷 조립을 위해 초기화
			_Client->PacketBytes = 0;

			// 완성된 패킷을 채팅 처리 로직에 전달
			ProcessPacket(_Client, packet);
		}
	}
}

void CServer::ProcessPacket(FClient* _Client, const FChatPacket& _Packet)
{
	// chatMsg : 클라가 실제로 보낸 채팅 메시지, 
	EChatType chatType = _Packet.Type;
	const std::string chatMsg = _Packet.Message;
	std::string formatMsg;

	{
		std::lock_guard<std::mutex> lock(mClientMutex);
		// 배열에 저장된 클라 ID의 이름정보를 얻어옴.
		std::string& clientName = mClients[_Client->ID].second;

		// 만약 클라의 이름이 설정안됐다면
		if (clientName.empty())
		{
			chatType = EChatType::CHAT_TYPE_CONNECTED;

			clientName = chatMsg;
			formatMsg = "[" + chatMsg + "] 님이 입장하셨습니다.";
		}

		// 이미 있다면 메시지 앞에 이름을 붙인다.
		else
		{
			formatMsg = "[" + clientName + "] : " + chatMsg;
		}
	}

	// 메세지 매니저에 저장
	AddChatMsg(formatMsg);

	// 전체 클라에게 브로드 캐스트
	BroadCastPacket(CUtils::MakePacket(chatType, formatMsg, _Packet.SenderID, _Packet.TimeStamp));
}
