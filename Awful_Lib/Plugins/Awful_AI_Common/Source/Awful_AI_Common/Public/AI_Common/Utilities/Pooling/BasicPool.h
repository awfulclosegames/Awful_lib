// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <assert.h>


namespace Awful
{
	// The default pool. Simple, fast, limited
	// Good contiguous access, fast acquire. 
	// ONLY supports rollback of the most recently acquired (top of the stack) item.
	// Good for uses where you rarely do rollbacks

	template<typename PoolType>
	class StackPoolImplementation
	{
	protected:
		class PoolEntry : public PoolType
		{ };

		class ItemHandleImplementation
		{
		public:
			ItemHandleImplementation() {}
			bool IsValid() const { return mPoolIndex >= 0; }
			bool operator==(const ItemHandleImplementation& aRHS) const { return mPoolIndex == aRHS.mPoolIndex; }
			bool operator!=(const ItemHandleImplementation& aRHS) const { return !(*this == aRHS); }
			bool operator<(const ItemHandleImplementation& aRHS) const { return mPoolIndex < aRHS.mPoolIndex; }
		private:
			friend class StackPoolImplementation<PoolType>;
			ItemHandleImplementation(unsigned int aIndex)
				:mPoolIndex(aIndex)
			{
			}
			unsigned int mPoolIndex = -1;
		};


	public:
		using PoolDataType = PoolType;


		StackPoolImplementation(unsigned int aSize = 64)
		{
			Reserve(aSize);
		}

		void Reserve(int aSize)
		{
			mPool.resize(aSize);
		}

		void ReserveIncrease(int aAdditional)
		{
			int newSize = (mInsertionPoint + aAdditional);
			if (newSize >= mPool.size())
			{
				// if we need to resize then let's try not to do it frequently
				newSize *= 2;
				mPool.resize(newSize);
			}
		}

		ItemHandleImplementation Acquire()
		{
			if (mInsertionPoint >= mPool.size())
			{
				auto newSize = mPool.size() * 2;
				mPool.resize(newSize);
			}
			unsigned int acquired = mInsertionPoint;
			++mInsertionPoint;

			// clear on acquisition	
			auto& entry = mPool[acquired];
			if (entry.NeedsClear)
			{
				entry.Clear();
				entry.NeedsClear = false;
			}

			return ItemHandleImplementation(acquired);
		}

		void Release(ItemHandleImplementation&& aItem)
		{
			assert(false && "Release not supported on Stack Pool");
		}

		void Release(ItemHandleImplementation& aItem)
		{
			assert(false && "Release not supported on Stack Pool");
		}

		void RollBack()
		{
			if (mInsertionPoint > 0)
			{
				--mInsertionPoint;
				mPool[mInsertionPoint].NeedsClear = true;
			}
		}

		PoolDataType& Edit(ItemHandleImplementation aIDX) { return mPool[aIDX.mPoolIndex]; }
		const PoolDataType& Read(ItemHandleImplementation aIDX) const { return mPool[aIDX.mPoolIndex]; }

		void Reset()
		{
			mInsertionPoint = 0;
		}

	private:
		class PoolStorageEntry : public PoolDataType
		{
		private:
			friend class StackPoolImplementation<PoolType>;
			
			bool NeedsClear = false;
		};

		unsigned int mInsertionPoint = 0;
		std::vector<PoolStorageEntry> mPool;
	};

	// TODO: implement a standard interface in the standard pool that calls through to the implementation
	template<class PoolType, class Pool_Implementation = StackPoolImplementation<PoolType>>
	class StandardPool : public Pool_Implementation
	{
	private:
		using BASE = Pool_Implementation;
	public:
		using ItemHandle = typename Pool_Implementation::ItemHandleImplementation;
		using PoolDataType = typename Pool_Implementation::PoolDataType;
		StandardPool(unsigned int aSize = 64)
			: BASE(aSize)
		{}

		//***************************************************************
		// assume that the implementation provides these functions:
		// Read()
		// Edit()
		//***************************************************************
		// standard pool interface

		PoolDataType& operator[](ItemHandle aIDX) { return BASE::Edit(aIDX); }
		const PoolDataType& operator[](ItemHandle aIDX) const { return BASE::Read(aIDX); }


	};
}
