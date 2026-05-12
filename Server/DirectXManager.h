#pragma once

#include <d3d11.h>
#include "DefineHeaders.h"
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
	void End();

public:
	ID3D11Device* GetDevice() const { return mDevice; }
	ID3D11DeviceContext* GetContext() const { return mContext; }

private:
	HWND mhWnd = 0;
	ID3D11Device* mDevice = nullptr;
	ID3D11DeviceContext* mContext = nullptr;
	IDXGISwapChain* mSwapChain = nullptr;
	ID3D11RenderTargetView* mRenderTargetView = nullptr;
};

