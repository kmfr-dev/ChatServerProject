#include "GUIManager.h"
#include "ChatServerApp.h"
#include "MessageManager.h"
#include "WindowManager.h"
#include "Client.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


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
			client->SetSerrverIP(NewServerIP);
			client->ConnectToServer();
		}
	}

	// 서버 IP 및 이름이 있다면 여기서는 데이터송수신
	else
	{
		ImGui::Text("Chat Start!");

		char ChatText[PACKET_SIZE] = { 0, };
		
		std::string NameText = "Your Name : ";
		NameText += CChatServerApp::GetInstance()->GetClient()->GetName();

		// 자신 이름 녹색으로 변경
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, NameText.c_str());

		// 채팅을 보냄 : 릴리즈일때만
#ifndef _DEBUG 
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
#else
		CMessageManager* MsgManager = CChatServerApp::GetInstance()->GetMessageManager();
		
		std::atomic<long long> RTT = MsgManager->GetRTT();
		std::atomic<long long> SCount = MsgManager->GetCount();

		double AverageRtt = (double)RTT.load() / SCount.load() / 1000.0;

		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), std::string(std::to_string(AverageRtt) + "ms").c_str());
#endif


	}

	ImGui::End();
}
