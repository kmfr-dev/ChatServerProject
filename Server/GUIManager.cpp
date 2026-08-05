#include "GUIManager.h"
#include "ChatServerApp.h"
#include "Server.h"
#include "WindowManager.h"
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

	// 디렉터리 정보에 폰트 경로 추가
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

void CGUIManager::RenderChat(const CChatServerApp& _App, const std::deque<std::string>& _ChatList)
{
	CServer* server = _App.GetServer();
	if (nullptr == server)
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
	
	ImGui::Begin("Chat Server", nullptr, window_flags);
	
	// 릴리즈일때만 채팅로그 출력
#ifndef _DEBUG
	ImGui::Text("Log");

	for (int i = 0; i < _ChatList.size(); ++i)
	{
		ImGui::Text(_ChatList[i].c_str());
	}

#else
	// 디버그일 때는 부하테스트 관련 텍스트 출력
	SERVER_LOADTEST_CHAT(_App);

	ImGui::Text("Connected Clients : %lld", server->GetConnectedClientCount());
	ImGui::Text("Pending Sends : %lld", server->GetQueuedPacketCount());
	ImGui::Text("Dropped Sends/sec : %.1f", mDroppedPerSecond);
	ImGui::Text("Dropped Sends Total : %lld", server->GetDroppedSendCount());
#endif

	ImGui::End();
}

void CGUIManager::SERVER_LOADTEST_CHAT(const CChatServerApp& _App)
{
	CServer* server = _App.GetServer();
	if (nullptr == server)
		return;

	constexpr long long UPDATE_INTERVAL_MS = 1000;
	const long long curTime = CTimerManager::GetInstance()->GetCurrentMS();

	const long long elapsedTime = curTime - mLastUpdateTime;

	if (elapsedTime < UPDATE_INTERVAL_MS)
		return;

	const long long curDroppedCnt = server->GetDroppedSendCount();
	const double elapsedSec = static_cast<double>(elapsedTime) / UPDATE_INTERVAL_MS;

	mDroppedPerSecond = static_cast<double>(curDroppedCnt - mPrevDroppedCount) / elapsedSec;
	mPrevDroppedCount = curDroppedCnt;

	mLastUpdateTime = curTime;
}
