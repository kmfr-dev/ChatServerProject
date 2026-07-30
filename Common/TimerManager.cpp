#include "TimerManager.h"

LARGE_INTEGER CTimerManager::mSecond = {};
LARGE_INTEGER CTimerManager::mTime = {};
float CTimerManager::mDeltaTime = 0.f;
CTimerManager* CTimerManager::mInstance = nullptr;

bool CTimerManager::Init()
{
    QueryPerformanceFrequency(&mSecond);
    QueryPerformanceCounter(&mTime);

    return true;
}

float CTimerManager::UpdateTick()
{
	LARGE_INTEGER	Time;
	QueryPerformanceCounter(&Time);

	mDeltaTime = (Time.QuadPart - mTime.QuadPart) / (float)mSecond.QuadPart;
	mTime = Time;

	return mDeltaTime;
}

long long CTimerManager::GetCurrentMS()
{
	LARGE_INTEGER Time;
	QueryPerformanceCounter(&Time);

	double ms = static_cast<double>(Time.QuadPart) / 
		static_cast<double>(mSecond.QuadPart)* 1000.0;

	return static_cast<long long>(ms);
}