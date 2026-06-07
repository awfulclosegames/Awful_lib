// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once


#include <unordered_map>
#include "Solver/WorldState.h"
#include "Utilities/Pooling/ChunkPool.h"
#include "Utilities/Pooling/SharedChunkPool.h"

// the structure of this whole file is a total mess. these classes need a desperate refactor!
// this one lives up to the namespace!

namespace Awful
{
	class WorldStateStack;

	struct WorldStateStackDataBlock
	{

		void Clear()
		{
			Parent = -1;
			State.Reset();
		}

		int Parent = -1;
		WorldState State;
	};


// ................................................................................
//  WorldState DataBlock Pooling
// ................................................................................

	// A pooling system for managing WorldStateStackDataBlock instances
	// to reduce memory allocations during planning operations.
	//
	// This pool maintains a free list of pre-allocated blocks that can be
	// rapidly reused when WorldStateStack::AcquireLayer() is called.
	// When a layer is no longer needed (e.g., after expanding candidates),
	// it can be returned to the pool for future reuse.
	class WorldStateStackDataBlockPool : public SharedChunkedPool<WorldStateStackDataBlock, 1024>
	{
	private:
		using BASE = SharedChunkedPool<WorldStateStackDataBlock, 1024>;
		public:
		WorldStateStackDataBlockPool(unsigned int aSize)
			: BASE(aSize)
		{}

		ItemHandle AcquireBlock(int aParentID = -1)
		{
			ItemHandle block = Acquire();
			Edit(block).Parent = aParentID;
			return block;
		}
	};

	using WSStackDataType = std::unordered_map<int, WorldStateStackDataBlockPool::ItemHandle>;

// ................................................................................
//  WorldStateStackIterator
// ................................................................................

	// there's a lot of duplication between the const and non-const iterators.
	// TODO: refactor these, possibly as a generic class since the logic is completely
	// identical, just with const types
	class WorldStateStackIterator
	{
	public:
		using StackIteratorType = typename WSStackDataType::iterator;
		WorldState::reference& operator*() { return *mWSIter; }
		WorldState::pointer operator->() { return mWSIter.operator->(); }

		WorldStateStackIterator& operator++();

		bool operator==(const WorldStateStackIterator& aRHS) const
		{
			return aRHS.mStackIter == mStackIter && aRHS.mStack == mStack;
		}

		bool operator!=(const WorldStateStackIterator& aRHS) const
		{
			return !(*this == aRHS);
		}

	private:
		friend class WorldStateStack;
		WorldStateStackIterator(WorldStateStack* aStack, StackIteratorType aStackIter);
		WorldStateStackIterator(WorldStateStack* aStack, StackIteratorType aStackIter, WorldState::iterator aWSIter);

		WorldStateStack* mStack;
		StackIteratorType mStackIter;
		WorldState::iterator mWSIter;
	};




	class Const_WorldStateStackIterator 
	{
	public:
		using ConstStackIteratorType = typename WSStackDataType::const_iterator;
		WorldState::const_reference& operator*() { return *mWSIter; }
		WorldState::const_pointer operator->() { return mWSIter.operator->(); }

		Const_WorldStateStackIterator& operator++();

		bool operator==(const Const_WorldStateStackIterator& aRHS) const
		{
			return aRHS.mStackIter == mStackIter && aRHS.mStack == mStack;
		}
		bool operator!=(const Const_WorldStateStackIterator& aRHS) const
		{
			return !(*this == aRHS);
		}

	private:
		friend class WorldStateStack;
		Const_WorldStateStackIterator(const WorldStateStack* aStack, ConstStackIteratorType aStackIter);
		Const_WorldStateStackIterator(const WorldStateStack* aStack, ConstStackIteratorType aStackIter, WorldState::const_iterator aWSIter);
	
		const WorldStateStack* mStack;
		ConstStackIteratorType mStackIter;
		WorldState::const_iterator mWSIter;

	};






	// ................................................................................
	//  WorldStateStack
	// ................................................................................

	class WorldStateStack
	{
	public:
		using iterator = WorldStateStackIterator;
		using const_iterator = Const_WorldStateStackIterator;

		// Enable pooling for this WorldStateStack instance
		// Set to false to disable pooling and use raw allocations
		WorldStateStack();

		// this shouldn't be in here but I get into a ton of reference problems if it's outside!
		// also this accessor is here to try and prevent direct access. It's probably a bit overengineered to be honest
		class WorldStateAccessor
		{
		public:
			std::size_t GetHash() const;
			// does not check for collisions that exist at lower levels of the stack
			void insertExact(const Condition& aCond);

			void insert(const Condition& aCond);
			void insert(const Observable& aObs);
			void insert(std::pair<const PoolString, bool>& aObs);

			WorldStateStack::iterator find(const Observable& aObs);
			WorldStateStack::const_iterator find(const Observable& aObs) const;
			int size() const;

			bool contains(const Observable& aObs) const;

			// return a new accessor with the current one as its parent
			WorldStateAccessor AcquireLayer() { return mStack->AcquireLayer(*this); }

			void Reset();
			bool operator==(const WorldStateAccessor& aRHS) const;
			bool operator!=(const WorldStateAccessor& aRHS) const { return !(*this == aRHS); }

			WorldStateStack::iterator begin();
			WorldStateStack::const_iterator begin() const;
			WorldStateStack::iterator end() { return mStack->end(); }
			WorldStateStack::const_iterator end() const { return mStack->cend(); }

			WorldStateAccessor() {}
		private:
			friend class WorldStateStack;
			WorldStateAccessor(int aRef, WorldStateStack& aStack)
				: mStack(&aStack)
				, mIndex(aRef)
			{
			}

			// simplified find to be used when inserting to avoid double inserts
			bool AlreadyExists(WSStackDataType::iterator aSearchLeaf, const Observable& aObs);
			bool AlreadyExists(WSStackDataType::iterator aSearchLeaf, std::pair<const PoolString, bool>& aObs);

			int GetId() const { return mIndex; }

			static const int INVALID = -1;
			WorldStateStack* mStack = nullptr;;
			int mIndex = INVALID;
		};



		WorldStateAccessor AcquireLayer();

		WorldStateStackIterator begin();
		Const_WorldStateStackIterator begin() const;
		WorldStateStackIterator end() { return mEndIter; }
		Const_WorldStateStackIterator cend() const { return mCEndIter; }
		void ResetCache() { mCache.clear(); }
		void clear() 
		{
			mInternalStack.clear(); 
			mCache.clear(); 
			mDataBlockPool.Clear();
		}
		void EnableCaching(bool aEnable) { mIsCacheEnabled = aEnable; }

	private:
		friend class WorldStateAccessor;
		friend class WorldStateStackIterator;
		friend class Const_WorldStateStackIterator;
		

		WorldStateAccessor AcquireLayer(const WorldStateAccessor& aParent);

		iterator MakeIterator(WSStackDataType::iterator aStackIter, WorldState::iterator aLayerIter);
		const_iterator MakeConstIterator(WSStackDataType::iterator aStackIter, WorldState::iterator aLayerIter) const;

		// caching here is dangerous, it is only valid in the GOAP usecase during the
		// add neighbours stage. During this stage we query if other action's pre/post conditions
		// are represented in the current action's expected worldstate, which remains unchanged.
		// a more general / safer caching scheme would be a real improvement later
		bool mIsCacheEnabled = false;
		// need a three way cache true/false/not-present
		std::unordered_map<Observable::KeyType, int8_t> mCache;

		int mInsertionPoint = 0;
		WSStackDataType mInternalStack;
		WorldStateStackIterator mEndIter;
		Const_WorldStateStackIterator mCEndIter;
		WorldStateStackDataBlockPool mDataBlockPool;
	};

}