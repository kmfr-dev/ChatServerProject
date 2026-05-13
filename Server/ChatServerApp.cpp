#include "ChatServerApp.h"
#include "WindowManager.h"
#include "DirectXManager.h"
#include "Server.h"
#include "GUIManager.h"

CChatServerApp* CChatServerApp::mInstance = nullptr;

CChatServerApp::CChatServerApp()
{
}

CChatServerApp::~CChatServerApp()
{
	// TODO Destructor
	Shutdown();
}

bool CChatServerApp::Init(HINSTANCE _hInstance)
{
	mWindowManager = new CWindowManager;
	if (!mWindowManager->Init(_hInstance))
	{
		delete mWindowManager;
		mWindowManager = nullptr;

		return false;
	}

	mDirectXManager = new CDirectXManager;
	if (!mDirectXManager->Init(mWindowManager->GethWnd()))
	{
		mWindowManager->UnRegisterWindowClass();

		delete mDirectXManager;
		mDirectXManager = nullptr;

		return false;
	}
	
	mWindowManager->ShowWindow();

	mGUIManager = new CGUIManager;
	if (!mGUIManager->Init(mWindowManager->GethWnd(), mDirectXManager->GetDevice(), 
		mDirectXManager->GetContext()))
	{
		delete mGUIManager;
		mGUIManager = nullptr;

		return false;
	}

	return true;
}

void CChatServerApp::Run()
{
	bool done = false;

	while (!done)
	{
		done = mWindowManager->MessageLoop();

		if (done)
			break;

		ResizeWindow();

		mDirectXManager->StartFrame();
		mGUIManager->StartFrame();

		// TODO 클라이언트에게 받은 채팅 렌더

		WaitForMultipleObjects()
		mGUIManager->RenderTest();

		mGUIManager->EndFrame();
		mDirectXManager->EndFrame();
	}
}

void CChatServerApp::Shutdown()
{
	if (mGUIManager)
	{
		mGUIManager->Shutdown();

		delete mGUIManager;
		mGUIManager = nullptr;
	}

	if (mDirectXManager)
	{
		mDirectXManager->Shutdown();

		delete mDirectXManager;
		mDirectXManager = nullptr;
	}

	if (mWindowManager)
	{
		mWindowManager->Shutdown();

		delete mWindowManager;
		mWindowManager = nullptr;
	}
}

void CChatServerApp::ResizeWindow()
{
	mDirectXManager->ResizeWindow(mWindowManager->GetResizeWidth(),
		mWindowManager->GetResizeHeight());

	mWindowManager->ClearResizeResoultion();
}
