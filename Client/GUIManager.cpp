#include "GUIManager.h"
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

void CGUIManager::RenderChat(const std::vector<std::string>& _ChatList)
{
	ImGui::Begin("Chat Server");
	ImGui::Text("Log");

	for (int i = 0; i < _ChatList.size(); ++i)
	{
		ImGui::Text(_ChatList[i].c_str());
	}

	ImGui::End();
}
