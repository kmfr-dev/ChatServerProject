#pragma once

#include "Define.h"

class CTimerManager
{
private:
	CTimerManager() {};
	~CTimerManager() {};

public:
	static CTimerManager* GetInstance()
	{
		if (nullptr == mInstance)
			mInstance = new CTimerManager;

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
	bool Init();
	float UpdateTick();
	long long GetCurrentMS();

private:
	static CTimerManager* mInstance;

private:
	static LARGE_INTEGER	mSecond;
	static LARGE_INTEGER	mTime;
	static float	mDeltaTime;

};

