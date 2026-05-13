#pragma once

#include "DefineHeaders.h"

class CWindowManager;
class CDirectXManager;
class CGUIManager;
class CServer;

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

private:
	static CChatServerApp* mInstance;

private:
	CWindowManager* mWindowManager = nullptr;
	CDirectXManager* mDirectXManager = nullptr;
	CGUIManager* mGUIManager = nullptr;
	CServer* mServer = nullptr;
};

