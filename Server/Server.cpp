#include "Server.h"
#include "ChatServerApp.h"
#include "MessageManager.h"

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
	mRunning = false;
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

void CServer::RegisterRecv(CClient* _Client)
{
	if (nullptr == _Client)
		return;

	ZeroMemory(&_Client->BufferInfo.Overlapped, sizeof(WSAOVERLAPPED));

	_Client->BufferInfo.rwMode = IO_MODE::READ;
	_Client->BufferInfo.WSABuf.buf = _Client->BufferInfo.Buffer;
	_Client->BufferInfo.WSABuf.len = BUF_SIZE;

	DWORD flags = 0;
	WSARecv(_Client->ClientSock, &_Client->BufferInfo.WSABuf, 1, nullptr, &flags, &_Client->BufferInfo.Overlapped, nullptr);
}

void CServer::BroadCastMsg(EChatType _Type, const std::string& _Message)
{
	FChatPacket packet;
	packet.Type = _Type;
	strncpy_s(packet.Message, _Message.c_str(), PACKET_SIZE);

	mClientMutex.lock();

	// 전체 클라이언트에게 브로드 캐스트
	for (const std::pair<CClient*, std::string>& pair : mClients)
	{
		const CClient* client = pair.first;

		if (nullptr == client || INVALID_SOCKET == client->ClientSock)
			continue;

		// 송신용 데이터 동적할당, 수신 완료 후 워커 쓰레드에서 해제함.
		FBufferInfo* sendData = new FBufferInfo;
		ZeroMemory(sendData, sizeof(FBufferInfo));

		sendData->rwMode = IO_MODE::WRITE;
		
		// 버퍼에 패킷정보 복사
		memcpy(sendData->Buffer, &packet, sizeof(packet));
		sendData->WSABuf.buf = sendData->Buffer;
		sendData->WSABuf.len = sizeof(packet);

		DWORD sendBytes = 0;

		// 비동기 송신 함수 호출
		if (SOCKET_ERROR == WSASend(client->ClientSock, &sendData->WSABuf, 1, &sendBytes,
			0, &sendData->Overlapped, nullptr))
		{
			// 에러 발생시 위에서 동적할당한 데이터 메모리 헤제
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				delete sendData;
			}
		}
	}

	mClientMutex.unlock();
}

void CServer::AddChatMsg(const std::string& _Message)
{
	mClientMutex.lock();

	// 메시지 매니저 배열에 메세지 추가
	CMessageManager* MsgManager = CChatServerApp::GetInstance()->GetMessageManager();
	if (nullptr != MsgManager)
	{
		MsgManager->AddRecvChat(_Message);
	}

	mClientMutex.unlock();
}

void CServer::ExitClient(CClient* _Client)
{
	if (nullptr == _Client)
		return;

	// 뮤텍스 락
	mClientMutex.lock();

	int ID = _Client->ID;
	// 클라 이름 복사 해두기
	std::string clientName = mClients[ID].second;

	// 클라 소켓 닫기
	closesocket(_Client->ClientSock);
	_Client->ClientSock = INVALID_SOCKET;
	
	mClients[ID].second = "";
	mClients[ID].first = nullptr;
	mDeathIDs.insert(ID);

	delete _Client;

	// 뮤텍스 언락
	mClientMutex.unlock();

	// 나간 클라이언트 메시지 브로드 캐스트
	if (!clientName.empty())
	{
		std::string exitMsg = "[" + clientName + "] 님이 나가셨습니다.";
		AddChatMsg(exitMsg);   
		BroadCastMsg(EChatType::CHAT_TYPE_EXIT, exitMsg);
	}
}

void CServer::ShutdownAllClient()
{
	mClientMutex.lock();

	// 모든 클라 순회 - 소켓의 우아한 종료 
	for (const std::pair<CClient*, std::string>& pair : mClients)
	{
		if (nullptr == pair.first)
			continue;

		if (INVALID_SOCKET == pair.first->ClientSock)
			continue;

		shutdown(pair.first->ClientSock, SD_BOTH);
	}

	mClientMutex.unlock();
}

void CServer::ClearClient()
{
	mClientMutex.lock();

	// 모든 클라 순회 - 클라 소켓 닫기 및 메모리 해제
	for (std::pair<CClient*, std::string>& pair : mClients)
	{
		if (nullptr == pair.first)
			continue;

		closesocket(pair.first->ClientSock);
		
		delete pair.first;
		pair.first = nullptr;
	}

	mClientMutex.unlock();
}

void CServer::AcceptClient()
{
	while (mRunning)
	{
		// 클라 생성 후 연결 요청 수락
		CClient* NewClient = new CClient;
		NewClient->ClientSock = accept(mListeningSocket, (sockaddr*)&NewClient->Addr, &NewClient->AddrSize);

		if (INVALID_SOCKET == NewClient->ClientSock)
		{
			delete NewClient;
			continue;
		}
		
		int ID = 0;

		// 임계구역 들어가기전에 Lock
		mClientMutex.lock();

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
		CreateIoCompletionPort((HANDLE)NewClient->ClientSock, mhIOCP, (ULONG_PTR)NewClient, 0);

		// Recv 등록 -> 데이터가 오면 IOCP가 워커 쓰레드를 깨워줌
		RegisterRecv(NewClient);

		// 뮤텍스 언락
		mClientMutex.unlock();
	}
}

void CServer::ProcessIO()
{
	while (true)
	{
		DWORD		byetsRansferred = 0;
		CClient*	client = nullptr;
		WSAOVERLAPPED* overlapped = nullptr;

		// IOCP 완료큐에서 이벤트를 꺼낸다. 올 때까지 블로킹.
		BOOL result = GetQueuedCompletionStatus(mhIOCP, &byetsRansferred, (PULONG_PTR)&client, &overlapped, INFINITE);

		// 실패했다면
		if (FALSE == result)
		{
			if (nullptr == overlapped)
				break;

			if (nullptr != client)
				ExitClient(client);

			continue;
		}

		// 클라가 정상 종료 했다면
		if (0 == byetsRansferred)
		{
			ExitClient(client);

			continue;
		}

		// I/O 완료 정보를 얻어옴
		FBufferInfo* IOData = (FBufferInfo*)overlapped;

		// 만약 쓰기모드면, Server -> Client로 전송한 것이므로 
		// 동적할당한 송신데이터를 해제.
 		if (IOData->rwMode == IO_MODE::WRITE)
		{
			delete IOData;
			continue;
		}

		// 읽기 모드면, Client -> Server로 전송한 정보
		if (IOData->rwMode == IO_MODE::READ)
		{
			// 클라이언트가 보낸 패킷정보를 얻어옴
			FChatPacket* packet = (FChatPacket*)IOData->Buffer;

			EChatType chatType = packet->Type;

			// chatMsg : 클라가 실제로 보낸 채팅 메시지, 
			std::string chatMsg = packet->Message;
			std::string formatMsg = "";

			mClientMutex.lock();

			// 배열에 저장된 클라 ID의 이름정보를 얻어옴.
			std::string& clientName = mClients[client->ID].second;

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

			mClientMutex.unlock();

			// 메세지 매니저에 저장
			AddChatMsg(formatMsg);

			// 전체 클라에게 브로드 캐스트
			BroadCastMsg(chatType, formatMsg);

			// 다음 수신을 예약
			RegisterRecv(client);
		}
	}
}
