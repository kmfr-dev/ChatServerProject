#pragma once

#include <d3d11.h>
#include "DefineHeaders.h"
#include "imgui.h"

class CGUIManager
{

public:
	CGUIManager() {};
	~CGUIManager() {};

public:
	bool Init(HWND _hWnd, ID3D11Device* _Device, ID3D11DeviceContext* _Context);
	void Run();
	void End();

private:
	ImVec4 mClearColor = {};
};

