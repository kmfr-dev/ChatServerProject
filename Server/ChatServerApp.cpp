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
	if (mGUIManager->Init(mWindowManager->GethWnd(), mDirectXManager->GetDevice(), 
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
	// TODO ..
}

void CChatServerApp::Shutdown()
{
	if (mGUIManager)
	{
		mGUIManager->End();

		delete mGUIManager;
		mGUIManager = nullptr;
	}

	if (mDirectXManager)
	{
		mDirectXManager->End();

		delete mDirectXManager;
		mDirectXManager = nullptr;
	}

	if (mWindowManager)
	{
		mWindowManager->End();

		delete mWindowManager;
		mWindowManager = nullptr;
	}
}