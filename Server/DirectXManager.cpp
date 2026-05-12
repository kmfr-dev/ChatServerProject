#include "DirectXManager.h"
#include "WindowManager.h"
#include "ChatServerApp.h"

bool CDirectXManager::CreateDevice()
{
	// 스왑체인 세팅
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Width = 0;
	swapDesc.BufferDesc.Height = 0;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = mhWnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Windowed = TRUE;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;

	D3D_FEATURE_LEVEL fLevel;
	const D3D_FEATURE_LEVEL fLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, 
		fLevelArray,2, D3D11_SDK_VERSION, &swapDesc, &mSwapChain, &mDevice, &fLevel, &mContext);

	// 하드웨어를 사용할 수 없는 경우 WARP 소프트웨어 드라이버 사용
	if(result == DXGI_ERROR_UNSUPPORTED)
	{
		result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
			fLevelArray, 2, D3D11_SDK_VERSION, &swapDesc, &mSwapChain, &mDevice, &fLevel, &mContext);
	}
	
	if(result != S_OK)
	{
		return false;
	}

	CreateRenderTarget();

	return true;
}

void CDirectXManager::CleanupDevice()
{
	CleanupRenderTarget();

	if (mSwapChain)
	{
		mSwapChain->Release();
		mSwapChain = nullptr;
	}

	if (mContext)
	{
		mContext->Release();
		mContext = nullptr;
	}

	if(mDevice)
	{
		mDevice->Release();
		mDevice = nullptr;
	}
}

void CDirectXManager::CreateRenderTarget()
{
	// 스왑체인의 백버퍼를 꺼내 렌더 타겟으로 설정
	ID3D11Texture2D* backBuffer;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	mDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);
	backBuffer->Release();
}

void CDirectXManager::CleanupRenderTarget()
{
	if (mRenderTargetView)
	{
		mRenderTargetView->Release();
		mRenderTargetView = nullptr;
	}
}

bool CDirectXManager::Init(HWND _hWnd)
{
	mhWnd = _hWnd;

	// Init device
	if (!CreateDevice())
	{
		CleanupDevice();
		return false;
	}

	return true;
}

void CDirectXManager::End()
{
	CleanupDevice();
}