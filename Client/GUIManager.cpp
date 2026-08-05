#include "GUIManager.h"
#include "ChatServerApp.h"
#include "MessageManager.h"
#include "WindowManager.h"
#include "Client.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "TimerManager.h"


bool CGUIManager::Init(HWND _hWnd, ID3D11Device* _Device, ID3D11DeviceContext* _Context)
{
	if (nullptr == _hWnd || nullptr == _Device || nullptr == _Context)
		return false;

	// 링크된 imgui 라이브러리 버전이 헤더 버전과 맞는지 검사
	IMGUI_CHECKVERSION();
	// imgui 컨텍스트 생성
	ImGui::CreateContext();

	// imgui의 입력/설정 상태 구조체를 얻어온다.
	ImGuiIO& io = ImGui::GetIO();
	// 키보드로 UI 조작 가능하게 설정
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// 현재 실행 디렉터리 정보를 얻어옴
	char dir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, dir);

	std::string curPath = dir;
	curPath += "\\malgun.ttf";

	io.Fonts->AddFontFromFileTTF(curPath.c_str(), 20.0f, NULL, io.Fonts->GetGlyphRangesKorean());

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(_hWnd);
	ImGui_ImplDX11_Init(_Device, _Context);

#ifdef _DEBUG
	mLastUpdateTime = CTimerManager::GetInstance()->GetCurrentMS();
#endif

	return true;
}

void CGUIManager::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void CGUIManager::StartFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void CGUIManager::EndFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CGUIManager::RenderChat(const CChatServerApp& _App, const std::deque<FChatData>& _ChatList)
{

#ifdef _DEBUG

	RECT rect;

	GetClientRect(_App.GetWindowManager()->GethWnd(), &rect);
	float width = (float)(rect.right - rect.left);
	float height = (float)(rect.bottom - rect.top);

	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width - 10.0f, height - 90.0f), ImGuiCond_Always);

	CMessageManager* msgManager = CChatServerApp::GetInstance()->GetMessageManager();
	if (nullptr == msgManager)
		return;
	
	SERVER_LOADTEST_CHAT(_App);

	ImGui::Begin("Server Load Test");

	// 연결된 클라이언트 수
	ImGui::Text("Connected Clients : %d / %d", msgManager->GetConnectedClientCount(),
		static_cast<int>(_App.GetTestClients().size()));
	ImGui::Text("Echo Responses/sec : %lld", mResponsesPerSec);
	ImGui::Text("Average RTT (last 1s) : %.3f ms", mAverRTTMS);
	ImGui::Text("Client Send Errors : %lld", msgManager->GetSendErrorCount());

	ImGui::End();
#else
	CClient* client = _App.GetClient();
	if (nullptr == client)
		return;

	RECT rect;
	GetClientRect(_App.GetWindowManager()->GethWnd(), &rect);
	float width = (float)(rect.right - rect.left);
	float height = (float)(rect.bottom - rect.top);

	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width - 10.0f, height - 90.0f), ImGuiCond_Always);

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoMove;    
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus; 

	ImGui::Begin("Client", nullptr,window_flags);
	
	// 이름이 비어있다면 이름 입력 텍스트 활성화
	if (client->GetName().empty())
	{
		ImGui::Text("Please Input Your Name..");

		char NewName[PACKET_SIZE] = { 0, };

		if (ImGui::InputText(" ", NewName, IM_ARRAYSIZE(NewName), ImGuiInputTextFlags_EnterReturnsTrue) == true)
		{
			client->SetName(NewName);
		}
	}
	// 이름은 있고 IP가 비어있다면
	else if (client->GetServerIP().empty())
	{
		ImGui::Text("Please Input Server IP..");

		char NewServerIP[PACKET_SIZE] = { 0, };

		if (ImGui::InputText(" ", NewServerIP, IM_ARRAYSIZE(NewServerIP), ImGuiInputTextFlags_EnterReturnsTrue) == true)
		{
			client->SetServerIP(NewServerIP);
			client->ConnectToServer();
		}
	}

	else
	{
		ImGui::Text("Chat Start!");

		char ChatText[PACKET_SIZE] = { 0, };

		std::string NameText = "Your Name : ";
		NameText += CChatServerApp::GetInstance()->GetClient()->GetName();

		// 자신 이름 녹색으로 변경
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, NameText.c_str());

		// 채팅을 보냄
		if (ImGui::InputText(" ", ChatText, IM_ARRAYSIZE(ChatText), ImGuiInputTextFlags_EnterReturnsTrue) == true)
		{
			CChatServerApp::GetInstance()->GetMessageManager()->AddChat(EChatType::CHAT_TYPE_NORMAL, ChatText, true);
		}
		CChatServerApp::GetInstance()->GetMessageManager()->GetMutex().lock();

		const std::deque<FChatData>& Chats = CChatServerApp::GetInstance()->GetMessageManager()->GetRecvChats();

		for (const FChatData& Chat : Chats)
		{
			ImVec4 Color = {};

			switch (Chat.ChatType)
			{
			case EChatType::CHAT_TYPE_ERROR:
			case EChatType::CHAT_TYPE_EXIT:
				Color = { 1.0f, 0.0f, 0.0f, 1.0f };
				break;
			case EChatType::CHAT_TYPE_CONNECTED:
				Color = { 0.0f, 1.0f, 0.0f, 1.0f };
				break;
			case EChatType::CHAT_TYPE_NORMAL:
			default:
				Color = { 1.0f, 1.0f, 1.0f, 1.0f };
				break;
			}

			ImGui::TextColored(Color, Chat.Message.c_str());
		}

		CChatServerApp::GetInstance()->GetMessageManager()->GetMutex().unlock();
	}

	ImGui::End();
#endif
}


void CGUIManager::SERVER_LOADTEST_CHAT(const CChatServerApp& _App)
{
	CMessageManager* msgManager = _App.GetMessageManager();
	if (nullptr == msgManager)
		return;

	constexpr long long UPDATE_INTERVAL_MS = 1000;
	const long long curTime = CTimerManager::GetInstance()->GetCurrentMS();
	const long long elapsedTime = curTime - mLastUpdateTime;

	if (elapsedTime < UPDATE_INTERVAL_MS)
		return;

	const FRTTStats stats = msgManager->GetIntervalRTT();
	const double elapsedSec = static_cast<double>(elapsedTime) / UPDATE_INTERVAL_MS;

	mResponsesPerSec = static_cast<long long>
		(static_cast<double>(stats.ResponseCnt) / elapsedSec);

	mAverRTTMS = stats.ResponseCnt > 0 ? 
		static_cast<double>(stats.TotalRTT) / stats.ResponseCnt : 0.0;
	
	mLastUpdateTime = curTime;
}