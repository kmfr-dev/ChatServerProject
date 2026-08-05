#pragma once

#include "../Common/DefineHeaders.h"

class CWindowManager;
class CDirectXManager;
class CGUIManager;
class CMessageManager;
class CClient;


class CChatServerApp
{
private:
	CChatServerApp();
	~CChatServerApp();

public:
	static CChatServerApp* GetInstance()
	{
		if (nullptr == mInstance)
			mInstance = new CChatServerApp;

		return mInstance;
	}

	static void DestroyInstance()
	{
		if (mInstance)
		{
			delete mInstance;
			mInstance = nullptr;
		}
	}

public:
	bool Init(HINSTANCE _hInstance);
	void Run();
	void Shutdown();

public:
	void ResizeWindow();

public:
	CWindowManager* GetWindowManager() const { return mWindowManager; }
	CDirectXManager* GetDirectXManager() const { return mDirectXManager; }
	CGUIManager* GetGUIManager() const { return mGUIManager; }
	CMessageManager* GetMessageManager() const { return mMessageManager; }
	CClient* GetClient() const { return mClient; }

private:
	static CChatServerApp* mInstance;

private:
	CWindowManager* mWindowManager = nullptr;
	CDirectXManager* mDirectXManager = nullptr;
	CGUIManager* mGUIManager = nullptr;
	CMessageManager* mMessageManager = nullptr;
	CClient* mClient = nullptr;


	// ============== SERVER LOAD TEST ==============
private:
	std::vector<CClient*> mTestClients;

public:
	const std::vector<CClient*>& GetTestClients() const { return mTestClients; }

private:
	void SERVER_TEST_ShutdownClients();

	// ==============================================
};

