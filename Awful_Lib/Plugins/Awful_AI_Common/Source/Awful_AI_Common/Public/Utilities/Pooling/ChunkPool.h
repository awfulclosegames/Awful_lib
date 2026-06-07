// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <list>
#include <cmath>
#include <array>
#include <assert.h>

#include "Utilities/Pooling/BasicPool.h"

namespace Awful
{
	template<class PoolType, unsigned int AllocationBlockSize, class ChunkStorageProvider>
	class ChunkedPoolImplementation;

	// generalizing storage so we can have a chunked pool that manages the store itself or a pool that acquires the 
	// storage blocks form another (more generally) shared pool without needing to duplicate a bunch of logic

	template<class PoolType, unsigned int AllocationBlockSize>
	class BaseChunkStorage
	{
	protected:
		class PoolStorageEntry : public PoolType
		{
		private:
			template<class , unsigned int , class >
			friend class ChunkedPoolImplementation;
			void Clear()
			{
				if (NeedsClear)
				{
					PoolType::Clear();
					mPoolIndex = -1;
					mSubIndex = -1;
					NeedsClear = false;
				}
			}

			// TODO: these could be bitranged, but the complexity of determining the correct ranges
			// from template parameters and then packing them into the correct bits is not worth the
			// memory savings at this point, especially since we are using arrays which have a fixed 
			// size and thus we can easily determine the correct ranges for the indices
			unsigned int mPoolIndex = -1;
			unsigned int mSubIndex = -1;
			// really only false the first time we acquire a slot, but once it has been acquired once we
			// we need to clear every other acquisition. We don't clear on release because there's no 
			// point paying for the clear until we try and acquire the slot again
			bool NeedsClear = false;
		};

		using PoolBlock = std::array<PoolStorageEntry, AllocationBlockSize>;
	};

	template<class PoolType, unsigned int AllocationBlockSize>
	class ChunkStorageLocal : public BaseChunkStorage<PoolType, AllocationBlockSize>
	{
		using BASE = BaseChunkStorage<PoolType, AllocationBlockSize>;
		// the interface here is a bit weak, since I expect it only to be used inside ChunkedPoolImplementation
	protected:
		using PoolBlock = BASE::PoolBlock;
		using PoolStorageEntry = BASE::PoolStorageEntry;

		PoolBlock* AcquireNewBlock()
		{
			mPoolStorage.emplace_back();
			return &mPoolStorage.back();
		}
	private:
		std::list<PoolBlock> mPoolStorage;
	};

	// TODO: use traits to ensure that the PoolType derives from BasicPool::PoolStorageHandle
	// we could avoid needing to require PoolType derive from BasicPool::PoolStorageHandle, but then
	// on release we would need to do a range check on the released item to see which pool block it was from
	// (or if it even comes from this pool). The range check is cheap, but we might need to perform it on 
	// a large number of blocks, so that is less appealing
	template<class PoolType, unsigned int AllocationBlockSize = 64, class ChunkStorageProvider = ChunkStorageLocal<PoolType, AllocationBlockSize>>
	class ChunkedPoolImplementation : private ChunkStorageProvider
	{
		using BASE = ChunkStorageProvider;

	protected:
		using PoolBlock = BASE::PoolBlock;
		using PoolStorageEntry = BASE::PoolStorageEntry;

		class ItemHandleImplementation
		{
		public:
			ItemHandleImplementation() {}
			bool IsValid() const { return mItemReference != nullptr; }
			bool operator==(const ItemHandleImplementation& aRHS) const { return mItemReference == aRHS.mItemReference; }
			bool operator!=(const ItemHandleImplementation& aRHS) const { return !(*this == aRHS); }
			bool operator<(const ItemHandleImplementation& aRHS) const { return mItemReference < aRHS.mItemReference; }
		private:
			friend class ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>;
			ItemHandleImplementation(PoolStorageEntry* aItem)
				: mItemReference(aItem)
			{}
			PoolStorageEntry* mItemReference;
		};

	public:
		using PoolDataType = PoolType;

		// the total pool size is number the closes multiple of AllocationBlockSize that will fit aSize
		ChunkedPoolImplementation(unsigned int aSize = 64)
		{
			//static_assert(IsPowerOfTwo(AllocationBlockSize), "Only powers of two are allowed for the allocation block size");
			mBlockSizePower = static_cast<unsigned int>(std::log2(AllocationBlockSize));
			Reserve(aSize);
		}


		void Reserve(unsigned int aSize)
		{
			size_t currentSize = Capacity();
			// already big enough
			if (aSize <= currentSize)
			{
				return;
			}
			size_t additionalSize = aSize - currentSize;
			ReserveIncrease(additionalSize);
		}

		void ReserveIncrease(size_t aAdditional)
		{
			if ((mCurrentCapacity - mCurrentSize)> aAdditional)
			{
				return;
			}

			// potentially overestimating, since we don't know that the current block is full, but this is a cheap calculation 
			// and we will likely need to resize again soon if we underestimate
			unsigned int blocksNeeded = static_cast<unsigned int>(std::ceil(static_cast<double>(aAdditional) / AllocationBlockSize));

			for (unsigned int i = 0; i < blocksNeeded; ++i)
			{
				PoolBlock* newBlock = BASE::AcquireNewBlock();
				mBlockAccessTable.push_back(newBlock);
			}

			mCurrentCapacity = static_cast<unsigned int>(mBlockAccessTable.size()) * AllocationBlockSize;
		}

		ItemHandleImplementation Acquire()
		{
			bool isFreeIndexEmpty = mHolesIndices.empty();
			if (isFreeIndexEmpty && mCurrentSize >= mCurrentCapacity)
			{
				// if we need to resize then let's try not to do it frequently
				size_t currentSize = Capacity();
				// double the size when we run out. At some point we might want 
				// to add an explicit grow factor and max size
				size_t additionalSize = currentSize;
				ReserveIncrease(additionalSize);
				// at some point check if we can't grow and if not fail by returning
				// an invalid handle
			}

			unsigned int acquiredIndex = mCurrentSize;
			// prefer reuse, if we have a free slot send it
			if (!isFreeIndexEmpty)
			{
				acquiredIndex = mHolesIndices.back();
				mHolesIndices.pop_back();
			}
			unsigned int blockIndex = acquiredIndex >> mBlockSizePower;
			unsigned int subIndex = acquiredIndex & (AllocationBlockSize - 1);
			PoolStorageEntry& entry = (*mBlockAccessTable[blockIndex])[subIndex];
			// clear only does anything if the entry was dirty to begin with
			entry.Clear();

			entry.mPoolIndex = blockIndex;
			entry.mSubIndex = subIndex;
			++mCurrentSize;
			return ItemHandleImplementation(&entry);
		}

		void Release(ItemHandleImplementation&& aItem)
		{
			if (aItem.IsValid())
			{
				// mark the slot as needing clear on next acquisition
				unsigned int blockIndex = (aItem.mItemReference->mPoolIndex << mBlockSizePower) + aItem.mItemReference->mSubIndex;
				mHolesIndices.push_back(blockIndex);
				aItem.mItemReference->NeedsClear = true;
				--mCurrentSize;
			}
		}

		void Release(ItemHandleImplementation& aItem)
		{
			if (aItem.IsValid())
			{
				// mark the slot as needing clear on next acquisition
				unsigned int blockIndex = (aItem.mItemReference->mPoolIndex << mBlockSizePower) + aItem.mItemReference->mSubIndex;
				mHolesIndices.push_back(blockIndex);
				aItem.mItemReference->NeedsClear = true;
				--mCurrentSize;
			}
		}


		void RollBack()
		{
			assert(false && "RollBack not supported on Chunked Pool");
		}

		PoolDataType& Edit(ItemHandleImplementation aIDX) { return *aIDX.mItemReference; }
		const PoolDataType& Read(ItemHandleImplementation aIDX) const { return *aIDX.mItemReference; }

		// Reset the pool to an empty state, making all slots available for acquisition again
		// returns the pool to the state it was in immediately after construction, but does not 
		// deallocate any memory. 
		// TODO: check if the clear here is better than just clearing on acquisition. 
		void Reset()
		{
			if (mCurrentSize > 0)
			{
				mHolesIndices.clear();
				for (auto currentBlock : mBlockAccessTable)
				{
					for (auto& entry : *currentBlock)
					{
						entry.Clear();
					}
				}
				mCurrentSize = 0;
			}
		}

		void Clear()
		{
			mHolesIndices.clear();
			mCurrentSize = 0;
		}


		// Get current size (number of allocated slots)
		unsigned int Size() const { return mCurrentSize; }

		// Get current capacity
		size_t Capacity() const { return mCurrentCapacity; }

	private:

		constexpr bool IsPowerOfTwo(int N) { return N && ((N & (N - 1)) == 0); }
		unsigned int mBlockSizePower = 0;
		
		// since we're using arrays not vectors, need to bookkeep this ourselves
		unsigned int mCurrentSize = 0;
		unsigned int mCurrentCapacity = 0;

		std::vector<PoolBlock*> mBlockAccessTable;
		std::vector<int> mHolesIndices;
	};


	// this is obnoxiously clumsy, and is needles duplication.
	// What I want to do here is a templated using so that I can pass in the 
	// allocation block size to the StandardPool specialization on ChunkedPoolImplementation, 
	// but I can't figure out how to do that without duplicating the whole class definition.
	template<class PoolType, unsigned int AllocationBlockSize = 64, class ChunkStorageProvider = ChunkStorageLocal<PoolType, AllocationBlockSize>>
	class ChunkedPool : public StandardPool<PoolType, ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>>
	{
	private:
		using BASE = StandardPool<PoolType, ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>>;
	public:
		ChunkedPool(unsigned int aSize = 64)
			: BASE(aSize)
		{}
	};
}

// just to support us putting these in unordered sets and maps, we need to be able to hash the handles.
// We can just use the pointer value since the handle is really just a wrapper around a pointer

template<class PoolType, unsigned int AllocationBlockSize, class ChunkStorageProvider>
struct std::hash<typename Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>>
{
	std::size_t operator()(const Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation& k) const noexcept
	{
		return std::hash<void*>{}(k.mItemReference);
	}
};

template<class PoolType, unsigned int AllocationBlockSize, class ChunkStorageProvider>
struct std::less<typename Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>>
{
	std::size_t operator()(const Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation& A, const Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation& B) const noexcept
	{
		return std::less < Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation >{}(A.mItemReference, B.mItemReference);
	}
};

template<class PoolType, unsigned int AllocationBlockSize, class ChunkStorageProvider>
struct std::equal_to<typename Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>>
{
	std::size_t operator()(const Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation& A, const Awful::ChunkedPoolImplementation<PoolType, AllocationBlockSize, ChunkStorageProvider>::ItemHandleImplementation& B) const noexcept
	{
		return A.mItemReference == B.mItemReference;
	}
};

