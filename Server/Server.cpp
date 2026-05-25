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
	serv_Addr.sin_port = htons(SERVER_PORT);

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
		mWorkerThreads.emplace_back(std::thread(&CServer::RecvChat, this));
	}
}

void CServer::RegisterRecv(CClient* _Client)
{
	if (nullptr == _Client)
		return;

	_Client->rwMode = IO_MODE::READ;
	_Client->WSABuf.buf = _Client->Buffer;
	_Client->WSABuf.len = BUF_SIZE;

	DWORD flags = 0;
	WSARecv(_Client->ClientSock, &_Client->WSABuf, 1, nullptr, &flags, &_Client->Overlapped, nullptr);
}

void CServer::BroadCastMsg(const std::string& _Message)
{
	mClientMutex.lock();

	// 전체 클라이언트에게 브로드 캐스트
	for (const std::pair<CClient*, std::string>& pair : mClients)
	{
		if (nullptr == pair.first)
			continue;

		if (INVALID_SOCKET == pair.first->ClientSock)
			continue;

		send(pair.first->ClientSock, _Message.c_str(), (int)_Message.size(), 0);
	}

	mClientMutex.unlock();
}

void CServer::AddChatMsg(const std::string& _Message)
{
	mClientMutex.lock();

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
	
	closesocket(_Client->ClientSock);
	_Client->ClientSock = INVALID_SOCKET;

	mClients[ID].first = nullptr;
	mDeathIDs.insert(ID);

	delete _Client;

	// 뮤텍스 언락
	mClientMutex.unlock();
}

void CServer::ShutdownAllClient()
{
	mClientMutex.lock();

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

void CServer::RecvChat()
{
	while (true)
	{
		DWORD		byetsRansferred = 0;
		CClient*	client = nullptr;
		OVERLAPPED* overlapped = nullptr;

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

		// 여기서 부터 정상 수신
		std::string chatMsg(client->Buffer, byetsRansferred);

		// 메세지 매니저에 저장
		AddChatMsg(chatMsg);

		// 전체 클라에게 브로드 캐스트
		BroadCastMsg(chatMsg);

		// 다음 수신을 예약
		RegisterRecv(client);
	}
}
