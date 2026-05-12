#pragma once

#include "DefineHeaders.h"
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
	void End();

public:
	void UnRegisterWindowClass();
	void ShowWindow();

public:
	HWND GethWnd() { return mhWnd; }

private:
	HINSTANCE mhInst = 0;
	HWND mhWnd = 0;
	WNDCLASSEXW mWC = {};

	WCHAR mTitleName[MAX_LOADSTRING] = {};
	WCHAR mClassName[MAX_LOADSTRING] = {};

	static UINT mResizeWidth;
	static UINT mResizeHeight;
};

