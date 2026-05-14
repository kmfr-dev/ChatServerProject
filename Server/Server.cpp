#include "Server.h"


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
	mAcceptThread = std::thread(&CServer::AcceptClient, this);

	// I/O처리를 담당할 쓰레드 생성
	InitThread();


	return 0;
}

void CServer::End()
{
	// TODO Shutdown 필요 - 소켓의 우아한 종료
	

	closesocket(mListeningSocket);

	// 현재 I/O를 처리하는 IOCP 완료 큐에 스레드 수만큼 종료 신호를 보냄
	// 그러면 I/O를 처리하는 쓰레드 전부가 종료 신호를 꺼내감
	for (int i = 0; i < mWorkerThreads.size(); ++i)
	{
		PostQueuedCompletionStatus(mhIOCP, 0, 0, nullptr);
	}

	// 만약 쓰레드가 종료가 안된상태로 
	// IOCP 핸들 및 소켓을 닫아버리면 리소스가 다 날아가버릴 수 있음
	mAcceptThread.join();
	for (std::thread& thread : mWorkerThreads)
		thread.join();

	
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

void CServer::AcceptClient()
{
	// TODO

}

void CServer::RecvChat()
{
	// TODO
}
