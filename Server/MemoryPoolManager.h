#pragma once

#include "Define.h"

struct FChunk
{
	friend class CMemoryPoolManager;

private:
	FChunk() : MemoryBlock(nullptr) {};
	~FChunk()
	{
		if (MemoryBlock)
		{
			// placement new를 사용하지 않았으므로 단순히 delete[]로 해제 가능
			// 만약 FBufferInfo에 생성자 로직이 복잡해지면 destructor 호출 필요
			delete[] MemoryBlock;
			MemoryBlock = nullptr;
		}
	}

private:
	bool PoolInit()
	{
		MemoryBlock = new FBufferInfo[POOL_SIZE];
		
		FreeStack.reserve(POOL_SIZE);
		for (int i = 0; i < POOL_SIZE; ++i)
		{
			FreeStack.emplace_back(&MemoryBlock[i]);
		}
		return true;
	}

	bool IsMyPointer(FBufferInfo* ptr)
	{
		return (ptr >= MemoryBlock && ptr < (MemoryBlock + POOL_SIZE));
	}

private:
	FBufferInfo* MemoryBlock;
	std::vector<FBufferInfo*> FreeStack;
};


class CMemoryPoolManager
{
private:
	CMemoryPoolManager();
	~CMemoryPoolManager();

public:
	static CMemoryPoolManager* GetInstance()
	{
		static CMemoryPoolManager Instance;
		return &Instance;
	}

public:
	bool Init();
	FBufferInfo* Get();
	void Release(FBufferInfo* buffer);

private:
	std::vector<FChunk*> mMemoryChunks;
	std::mutex mMutex;
};