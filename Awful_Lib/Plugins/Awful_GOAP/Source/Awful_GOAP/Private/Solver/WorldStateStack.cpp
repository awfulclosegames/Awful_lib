// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "Solver/WorldStateStack.h"
#include "Solver/WorldStateStackDataBlockPoolHolder.h"

namespace Awful
{


	// ................................................................................
	//  WorldStateStackIterator
	// .................................................................................



	WorldStateStackIterator::WorldStateStackIterator(WorldStateStack* aStack, WorldStateStackIterator::StackIteratorType aStackIter)
		: mStack(aStack)
		, mStackIter(aStackIter)
	{
		if (mStackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[mStackIter->second];
			mWSIter = dataBlock.State.begin();
		}
	}

	WorldStateStackIterator::WorldStateStackIterator(WorldStateStack* aStack, WorldStateStackIterator::StackIteratorType aStackIter, WorldState::iterator aWSIter)
		: mStack(aStack)
		, mStackIter(aStackIter)
		, mWSIter(aWSIter)
	{
	}



	WorldStateStackIterator& WorldStateStackIterator::operator++()
	{
		++mWSIter;
		auto* dataBlock = &mStack->mDataBlockPool[mStackIter->second];
		while (mWSIter == dataBlock->State.end())
		{
			mStackIter = mStack->mInternalStack.find(dataBlock->Parent);
			if (mStackIter == mStack->mInternalStack.end())
			{
				break;
			}
			// changed the stack iterator, need to access the next data block
			dataBlock = &mStack->mDataBlockPool[mStackIter->second];
			mWSIter = dataBlock->State.begin();
		}
		return *this;
	}


	Const_WorldStateStackIterator& Const_WorldStateStackIterator::operator++()
	{
		++mWSIter;
		auto dataBlock = &mStack->mDataBlockPool[mStackIter->second];
		while (mWSIter == dataBlock->State.end())
		{
			mStackIter = mStack->mInternalStack.find(dataBlock->Parent);
			if (mStackIter == mStack->mInternalStack.end())
			{
				break;
			}
			// changed the stack iterator, need to access the next data block
			dataBlock = &mStack->mDataBlockPool[mStackIter->second];
			mWSIter = dataBlock->State.begin();
		}
		return *this;
	}


	Const_WorldStateStackIterator::Const_WorldStateStackIterator(const WorldStateStack* aStack, ConstStackIteratorType aStackIter)
		: mStack(aStack)
		, mStackIter(aStackIter)
	{
		if (mStackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[mStackIter->second];
			mWSIter = dataBlock.State.begin();
		}
	}

	// seems hacky, but I know this won't allow non-const access since the operator*() returns a const iterator
	Const_WorldStateStackIterator::Const_WorldStateStackIterator(const WorldStateStack* aStack, ConstStackIteratorType aStackIter, WorldState::const_iterator aWSIter)
		: mStack(aStack)
		, mStackIter(aStackIter)
		, mWSIter(aWSIter)
	{
	}



	// ................................................................................
	//  WorldStateAccessor
	// .................................................................................

		// here I need to compute an accurate hash for the combined world state
		// so compute a rotated base hash xored with the correct hash for this layer
		// which should also accumulate recursively for all ancestor layers.
		// ALSO means I need to trim inserts to only items not existing in any ancestor
	std::size_t  WorldStateStack::WorldStateAccessor::GetHash() const
	{
		static std::hash<int> hasher;
		return hasher(mIndex);
	}

	void WorldStateStack::WorldStateAccessor::insertExact(const Condition& aCond)
	{
		auto currentLayer = mStack->mInternalStack.find(mIndex);
		if (currentLayer != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[currentLayer->second];
			auto& worldstateLayer = dataBlock.State;
			worldstateLayer.insert(aCond);
		}
	}

	void WorldStateStack::WorldStateAccessor::insert(const Condition& aCond)
	{
		auto currentLayer = mStack->mInternalStack.find(mIndex);
		if (currentLayer != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[currentLayer->second];
			auto& worldstateLayer = dataBlock.State;
			// loop through all observables in a condition and add them to the current worldstate
			for (auto& obs : aCond)
			{
				if (!AlreadyExists(currentLayer, obs))
				{
					worldstateLayer.insert(obs);
				}
			}
		}
	}

	void WorldStateStack::WorldStateAccessor::insert(const Observable& aObs)
	{
		auto currentLayer = mStack->mInternalStack.find(mIndex);
		if (currentLayer != mStack->mInternalStack.end())
		{
			if (!AlreadyExists(currentLayer, aObs))
			{
				auto& dataBlock = mStack->mDataBlockPool[currentLayer->second];
				auto& worldstateLayer = dataBlock.State;
				worldstateLayer.insert_or_assign(aObs.GetKey(), aObs.GetValue());
			}
		}
	}

	void WorldStateStack::WorldStateAccessor::insert(std::pair<const Observable::KeyType, Observable::ValueType>& aObs)
	{
		auto currentLayer = mStack->mInternalStack.find(mIndex);
		if (currentLayer != mStack->mInternalStack.end())
		{
			if (!AlreadyExists(currentLayer, aObs))
			{
				auto& dataBlock = mStack->mDataBlockPool[currentLayer->second];
				auto& worldstateLayer = dataBlock.State;
				worldstateLayer.insert_or_assign(aObs.first, aObs.second);
			}
		}
	}


	// this with the iterator management and stuff still feels pretty messed up.
	// First revision stuff, iterate on this to simplify it
	WorldStateStack::iterator WorldStateStack::WorldStateAccessor::find(const Observable& aObs)
	{
		auto stackIter = mStack->mInternalStack.find(mIndex);

		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			auto layerIter = dataBlock.State.find(aObs);

			if (layerIter != dataBlock.State.end())
			{
				return  mStack->MakeIterator(stackIter, layerIter);
			}
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}
		return mStack->end();
	}

	WorldStateStack::const_iterator WorldStateStack::WorldStateAccessor::find(const Observable& aObs) const
	{
		auto stackIter = mStack->mInternalStack.find(mIndex);

		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			auto layerIter = dataBlock.State.find(aObs);

			if (layerIter != dataBlock.State.end())
			{
				return  mStack->MakeConstIterator(stackIter, layerIter);
			}
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}
		return mStack->cend();
	}

	int WorldStateStack::WorldStateAccessor::size() const
	{
		int currentSize = 0;
		auto stackIter = mStack->mInternalStack.find(mIndex);
		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			currentSize += static_cast<int>(dataBlock.State.size());
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}

		return currentSize;
	}

	bool WorldStateStack::WorldStateAccessor::contains(const Observable& aObs) const
	{
		if (mStack->mIsCacheEnabled)
		{
			const auto& cached = mStack->mCache.find(aObs.GetKey());
			if (cached != mStack->mCache.end())
			{
				return (cached->second > -1) & (cached->second == static_cast<int8_t>(aObs.GetValue()));
			}
		}
		auto stackIter = mStack->mInternalStack.find(mIndex);
		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			const auto& iter = dataBlock.State.find(aObs);
			if (iter != dataBlock.State.end())
			{
				if (mStack->mIsCacheEnabled)
				{
					mStack->mCache.insert_or_assign(aObs.GetKey(), iter->second);
				}
				return iter->second == aObs.GetValue();
			}
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}

		if (mStack->mIsCacheEnabled)
		{
			// not found, we are cacheing dangerously. We 'know' for the GOAP use case
			// that during the add neightbours stage (which is the only place we allow
			// caching) that we do not add to the active node's worldstate
			// so if we see a missing observation we can cache it as missing
			mStack->mCache.insert_or_assign(aObs.GetKey(), -1);
		}
		return false;
	}

	bool WorldStateStack::WorldStateAccessor::AlreadyExists(WSStackDataType::iterator aSearchLeaf, const Observable& aObs)
	{
		auto stackIter = aSearchLeaf;

		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			auto layerIter = dataBlock.State.find(aObs);

			if (layerIter != dataBlock.State.end())
			{
				return layerIter->second == aObs.GetValue();
			}
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}
		return false;
	}

	// I bet I could do this without all the duplicated code but I really can't be bothered. I'm half tempted to delete the std::pair<>
	// versions of the api here anyway
	bool WorldStateStack::WorldStateAccessor::AlreadyExists(WSStackDataType::iterator aSearchLeaf, std::pair<const Observable::KeyType, Observable::ValueType>& aObs)
	{
		auto stackIter = aSearchLeaf;

		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			auto layerIter = dataBlock.State.find(aObs.first);

			if (layerIter != dataBlock.State.end())
			{
				return layerIter->second == aObs.second;
			}
			stackIter = mStack->mInternalStack.find(dataBlock.Parent);
		}
		return false;
	}

	void WorldStateStack::WorldStateAccessor::Reset()
	{

	}


	bool WorldStateStack::WorldStateAccessor::operator==(const WorldStateAccessor& aRHS) const
	{
		return (mIndex == aRHS.mIndex) && (&mStack == &aRHS.mStack);
	}


	WorldStateStack::iterator  WorldStateStack::WorldStateAccessor::begin()
	{
		auto stackIter = mStack->mInternalStack.find(mIndex);
		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			if (dataBlock.State.empty())
			{
				stackIter = mStack->mInternalStack.find(dataBlock.Parent);
				continue;
			}
			auto layerIter = dataBlock.State.begin();
			return  mStack->MakeIterator(stackIter, layerIter);
		}

		return mStack->end();
	}

	WorldStateStack::const_iterator  WorldStateStack::WorldStateAccessor::begin() const
	{
		auto stackIter = mStack->mInternalStack.find(mIndex);

		while (stackIter != mStack->mInternalStack.end())
		{
			auto& dataBlock = mStack->mDataBlockPool[stackIter->second];
			if (dataBlock.State.empty())
			{
				stackIter = mStack->mInternalStack.find(dataBlock.Parent);
				continue;
			}
			auto layerIter = dataBlock.State.begin();
			return  mStack->MakeConstIterator(stackIter, layerIter);
		}
		return mStack->cend();
	}

	// ................................................................................
	//  WorldStateStack
	// ................................................................................

	WorldStateStack::WorldStateStack()
		: mInternalStack()
		, mEndIter(this, mInternalStack.end())
		, mCEndIter(this, mInternalStack.end())
		, mDataBlockPool(64)
	{
	}

	// TODO: once the performance on the chunked pool is ok, then rework the acquire layer functions to us the pool instead of direct insertions. 
	// This will likely require a bit of reworking of the WorldStateAccessor to handle the fact that the pool returns handles instead of direct 
	// references, but it should be doable without too much hassle. 
	WorldStateStack::WorldStateAccessor WorldStateStack::AcquireLayer()
	{
		auto DataBlock = mDataBlockPool.AcquireBlock();
		auto newEntry = mInternalStack.insert(std::make_pair(mInsertionPoint, DataBlock));

		return WorldStateAccessor(mInsertionPoint++, *this);
	}

	WorldStateStack::WorldStateAccessor WorldStateStack::AcquireLayer(const WorldStateAccessor& aParent)
	{
		auto DataBlock = mDataBlockPool.AcquireBlock(aParent.GetId());
		auto newEntry = mInternalStack.insert(std::make_pair(mInsertionPoint, DataBlock));

		return WorldStateAccessor(mInsertionPoint++, *this);

	}

	WorldStateStack::iterator WorldStateStack::MakeIterator(WSStackDataType::iterator aStackIter, WorldState::iterator aLayerIter)
	{
		return WorldStateStackIterator(this, aStackIter, aLayerIter);
	}

	WorldStateStack::const_iterator WorldStateStack::MakeConstIterator(WSStackDataType::iterator aStackIter, WorldState::iterator aLayerIter) const
	{
		return Const_WorldStateStackIterator(this, aStackIter, aLayerIter);
	}

	WorldStateStack::iterator WorldStateStack::begin()
	{
		if (mInternalStack.size() == 0)
		{
			return end();
		}
		auto& dataBlock = mDataBlockPool[mInternalStack.begin()->second];

		return WorldStateStackIterator(this, mInternalStack.begin(), dataBlock.State.begin());
	}

	// a hack to work around a messed up const iterator. This is all backwards, rework this!
	WorldStateStack::const_iterator WorldStateStack::begin() const
	{
		if (mInternalStack.size() == 0)
		{
			return cend();
		}

		auto NC_stack = const_cast<WSStackDataType*>(&mInternalStack);

		auto& dataBlock = mDataBlockPool[NC_stack->begin()->second];
		return Const_WorldStateStackIterator(this, NC_stack->begin(), dataBlock.State.begin());
	}
}