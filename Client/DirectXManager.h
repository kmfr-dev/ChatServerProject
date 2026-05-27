#pragma once

#include "../Common/DefineHeaders.h"
#include "Define.h"

class CDirectXManager
{
public:
	CDirectXManager() {};
	~CDirectXManager() {};

private:
	bool CreateDevice();
	void CleanupDevice();
	void CreateRenderTarget();
	void CleanupRenderTarget();

public:
	bool Init(HWND _hWnd);
	void Shutdown();

public:
	void StartFrame();
	void EndFrame();

public:
	void ResizeWindow(UINT _ResizeWidth, UINT _ResizeHeight);

public:
	ID3D11Device* GetDevice() const { return mDevice; }
	ID3D11DeviceContext* GetContext() const { return mContext; }

private:
	HWND mhWnd = 0;
	ID3D11Device* mDevice = nullptr;
	ID3D11DeviceContext* mContext = nullptr;
	IDXGISwapChain* mSwapChain = nullptr;
	ID3D11RenderTargetView* mRenderTargetView = nullptr;

	ImVec4 mClearColor = {};
};

