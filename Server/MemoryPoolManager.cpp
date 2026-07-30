#include "MemoryPoolManager.h"

CMemoryPoolManager::CMemoryPoolManager()
{
}

CMemoryPoolManager::~CMemoryPoolManager()
{
	std::lock_guard<std::mutex> lock(mMutex);
	for (FChunk* chunk : mMemoryChunks)
	{
		delete chunk;
	}
	mMemoryChunks.clear();
}

bool CMemoryPoolManager::Init()
{
	std::lock_guard<std::mutex> lock(mMutex);
	mMemoryChunks.reserve(MEMORY_CHUNKSIZE);
	
	FChunk* Chunk = new FChunk;
	if (false == Chunk->PoolInit())
	{
		delete Chunk;
		return false;
	}

	mMemoryChunks.emplace_back(Chunk);

	return true;
}

FBufferInfo* CMemoryPoolManager::Get()
{
	// 사용전 락잡기
	std::lock_guard<std::mutex> lock(mMutex);
	
	for (FChunk* Chunk : mMemoryChunks)
	{
		if (Chunk->FreeStack.empty())
			continue;

		FBufferInfo* BufInfo = Chunk->FreeStack.back();
		Chunk->FreeStack.pop_back();

		// 사용 전 초기화
		memset(BufInfo, 0, sizeof(FBufferInfo));

		return BufInfo;
	}

	// 모든 청크가 가득 찼으면 새 청크 생성
	if (mMemoryChunks.size() >= MAX_MEMORY_CHUNKS)
		return nullptr;

	FChunk* NewChunk = new FChunk;
	if (!NewChunk->PoolInit())
	{
		delete NewChunk;
		return nullptr;
	}
	
	mMemoryChunks.emplace_back(NewChunk);

	FBufferInfo* PoolBuf = NewChunk->FreeStack.back();
	NewChunk->FreeStack.pop_back();
	
	memset(PoolBuf, 0, sizeof(FBufferInfo));

	return PoolBuf;
}

void CMemoryPoolManager::Release(FBufferInfo* buffer)
{
	if (nullptr == buffer)
		return;

	std::lock_guard<std::mutex> lock(mMutex);

	// 주소 범위를 확인하여 해당 포인터가 어느 청크 소속인지 판별하여 사용
	// EX) i번째 Chunk 소속인지 확인하여 참이면 메모리 반납
	for (FChunk* chunk : mMemoryChunks)
	{
		if (chunk->IsMyPointer(buffer))
		{
			chunk->FreeStack.emplace_back(buffer);
			return;
		}
	}

	// 만약 어느 청크에도 속하지 않는다면 외부에서 잘못 전달된 포인터
	assert(false && "Not InValid Ptr..!");
}
