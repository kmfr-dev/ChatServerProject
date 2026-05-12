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

	mClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	return true;
}

void CGUIManager::Run()
{

}

void CGUIManager::End()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}