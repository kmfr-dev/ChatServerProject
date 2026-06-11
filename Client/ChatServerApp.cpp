#include "ChatServerApp.h"
#include "WindowManager.h"
#include "DirectXManager.h"
#include "GUIManager.h"
#include "MessageManager.h"
#include "Client.h"
#include "TimerManager.h"

CChatServerApp* CChatServerApp::mInstance = nullptr;

CChatServerApp::CChatServerApp()
{
}

CChatServerApp::~CChatServerApp()
{
	Shutdown();
}

bool CChatServerApp::Init(HINSTANCE _hInstance)
{
	if (!CTimerManager::GetInstance()->Init())
	{
		CTimerManager::DestroyInstnace();
		return false;
	}

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

	//디버그일 때는 더미 클라이언트 생성
#ifdef _DEBUG

	mTestClients.reserve(MAX_CLIENT);
	for (int i = 0; i < MAX_CLIENT; ++i)
	{
		CClient* DummyClient = new CClient;
		DummyClient->SetName("DummyClient" + i);
		DummyClient->SetSerrverIP(SERVER_IP);

		mTestClients.emplace_back(DummyClient);
	}

#else
	mClient = new CClient;
	if (!mClient->Init())
	{
		delete mClient;
		mClient = nullptr;

		return false;
	}
#endif

	mMessageManager = new CMessageManager;
	if (!mMessageManager->Init())
	{
		delete mMessageManager;
		mMessageManager = nullptr;

		return false;
	}
	
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

		mGUIManager->RenderChat(*this, mMessageManager->GetRecvChats());

		mGUIManager->EndFrame();
		mDirectXManager->EndFrame();
	}
}

void CChatServerApp::Shutdown()
{
#ifdef _DEBUG
	SERVER_TEST_ShutdownClients();
#else
	if (mClient)
	{
		mClient->End();

		delete mClient;
		mClient = nullptr;
	}
#endif

	if (mMessageManager)
	{
		mMessageManager->End();
		mMessageManager->ClearChatList();

		delete mMessageManager;
		mMessageManager = nullptr;
	}

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

	CTimerManager::DestroyInstnace();
}

void CChatServerApp::ResizeWindow()
{
	mDirectXManager->ResizeWindow(mWindowManager->GetResizeWidth(),
		mWindowManager->GetResizeHeight());

	mWindowManager->ClearResizeResoultion();
}

// SERVER LOAD TEST

void CChatServerApp::SERVER_TEST_ShutdownClients()
{
	for (int i = 0; i < mTestClients.size(); ++i)
	{
		if (nullptr == mTestClients[i])
			continue;

		mTestClients[i]->End();

		delete mTestClients[i];
		mTestClients[i] = nullptr;
	}

	WSACleanup();
}