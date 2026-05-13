#pragma once

#include "DefineHeaders.h"

class CGUIManager
{

public:
	CGUIManager() {};
	~CGUIManager() {};

public:
	bool Init(HWND _hWnd, ID3D11Device* _Device, ID3D11DeviceContext* _Context);
	void Shutdown();

	void StartFrame();
	void EndFrame();
	void RenderTest();
};

