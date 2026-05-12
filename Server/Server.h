#pragma once

#include "resource.h"

class CServer
{
private:
	CServer() {};
	~CServer() {};

public:
	static CServer* GetInstance()
	{
		if (nullptr == mInstance)
			mInstance = new CServer;

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
	int		Start();
	void	End();

private:
	static CServer* mInstance;

};