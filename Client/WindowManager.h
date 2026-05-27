#pragma once

#include "../Common/DefineHeaders.h"
#include "Define.h"


class CWindowManager
{
public:
	CWindowManager() {};
	~CWindowManager() {};

private:
	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	bool Init(HINSTANCE _hInstance);
	bool MessageLoop();
	void Shutdown();

public:
	void UnRegisterWindowClass();
	void ShowWindow();
	void ClearResizeResoultion();

public:
	void StartFrame();
	void EndFrame();

public:
	HWND GethWnd() const { return mhWnd; }
	UINT GetResizeWidth() const { return mResizeWidth; }
	UINT GetResizeHeight() const { return mResizeWidth; }

private:
	HINSTANCE mhInst = 0;
	HWND mhWnd = 0;
	WNDCLASSEXW mWC = {};

	WCHAR mTitleName[MAX_LOADSTRING] = {};
	WCHAR mClassName[MAX_LOADSTRING] = {};

	static UINT mResizeWidth;
	static UINT mResizeHeight;

	float mMainScale = 0.f;
};

