#pragma once

#include "../Common/DefineHeaders.h"
#include "Define.h"

class CChatServerApp;

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
	void RenderChat(const CChatServerApp& _App, const std::deque<FChatData>& _ChatList);

	// SERVER LOAD TEST
private:
	long long mLastUpdateTime = 0;
	long long mResponsesPerSec = 0;
	double mAverRTTMS = 0.0;

	void SERVER_LOADTEST_CHAT(const CChatServerApp& _App);
};

