// Copyright Strati D. Zerbinis 2026. All Rights Reserved.

#pragma once

#include "AI_Common/Utilities/Pooling/ChunkPool.h"

namespace Awful
{


	// Global pool instance for shared access across all WorldStateStack instances
	// some irony in that it allocates a pool of poolblocks
	template<class PoolBlock>
	class SharedPoolChunkHolder
	{
		struct BlockType
		{
			PoolBlock Block;
			void Clear() {}
		};
		using PoolType = ChunkedPool<BlockType>;
	public:
		using ItemHandle = PoolType::ItemHandle;

		static PoolType& GetInstance()
		{
			static PoolType sInstance(32);
			return sInstance;
		}
		static ItemHandle Acquire()
		{
			ItemHandle item = GetInstance().Acquire();
			return item;
		}

		static void Release(ItemHandle& aItem)
		{
			auto preSize = GetInstance().Size();
			GetInstance().Release(aItem);

		}


		// Reset the global pool (call during shutdown or when rebuilding world)
		static void ResetGlobalPool()
		{
			GetInstance().Clear();
		}

	private:
		SharedPoolChunkHolder() = delete;
		~SharedPoolChunkHolder() = delete;
		SharedPoolChunkHolder(const SharedPoolChunkHolder&) = delete;
		SharedPoolChunkHolder& operator=(const SharedPoolChunkHolder&) = delete;
	};



	template<class PoolType, unsigned int AllocationBlockSize>
	class ChunkStorageShared : public BaseChunkStorage<PoolType, AllocationBlockSize>
	{
		using BASE = BaseChunkStorage<PoolType, AllocationBlockSize>;
		using SharedBlockSource = SharedPoolChunkHolder<typename BASE::PoolBlock>;
		using StorageType = SharedBlockSource::ItemHandle;
		// the interface here is a bit weak, since I expect it only to be used inside ChunkedPoolImplementation
	protected:
		using PoolBlock = BASE::PoolBlock;
		using PoolStorageEntry = BASE::PoolStorageEntry;

		~ChunkStorageShared()
		{
			for (auto item : mPoolReferanceStorage)
			{
				SharedBlockSource::Release(item);
			}

			mPoolReferanceStorage.clear();
		}

		PoolBlock* AcquireNewBlock()
		{
			auto item = SharedBlockSource::Acquire();
			mPoolReferanceStorage.push_back(item);
			return &SharedBlockSource::GetInstance()[item].Block;
		}
	private:
		std::vector<StorageType> mPoolReferanceStorage;
	};


	template<class PoolType, unsigned int AllocationBlockSize = 64>
	class SharedChunkedPool : public ChunkedPool<PoolType, AllocationBlockSize, ChunkStorageShared<PoolType, AllocationBlockSize>>
	{
	private:
		using BASE = ChunkedPool<PoolType, AllocationBlockSize, ChunkStorageShared<PoolType, AllocationBlockSize>>;
	public:
		SharedChunkedPool(unsigned int aSize = 64)
			: BASE(aSize)
		{
		}
	};
}