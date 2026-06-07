// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include <unordered_set>

#include "GraphOptimizer/SequenceUnit.h"

#include "ActionSpaceSearch/ActionMetadata.h"
#include "Solver/WorldStateStack.h"

template<>
struct std::hash<const Awful::GO::SequenceUnit<Awful::ActionSpaceSearch::ActionMetadata>*>
{
	using UnitType = Awful::GO::SequenceUnit<Awful::ActionSpaceSearch::ActionMetadata> ;
	std::size_t operator()(const UnitType* const& p) const noexcept
	{
		return  p->GetHash();
	}
};

template<>
struct std::equal_to<const Awful::GO::SequenceUnit<Awful::ActionSpaceSearch::ActionMetadata>*>
{
	using UnitType = Awful::GO::SequenceUnit<Awful::ActionSpaceSearch::ActionMetadata>;

	bool operator()(const UnitType* const& A, const UnitType* const& B) const
	{
		return A->GetExpectedWorld() == B->GetExpectedWorld();
	}
};


namespace Awful
{
	namespace ActionSpaceSearch
	{
		class GOAP_StateExtension
		{
		public:
			using SequenceUnit = GO::SequenceUnit<ActionMetadata>;

			GOAP_StateExtension()
				: mMetadataPool()
				, mSeen()
			{
			}
			GOAP_StateExtension(const GOAP_StateExtension& aRHS)
				: mMetadataPool(aRHS.mMetadataPool)
				, mSeen(aRHS.mSeen)
			{
			}
			GOAP_StateExtension(const GOAP_StateExtension&& aRHS)
				: mMetadataPool(std::move(aRHS.mMetadataPool))
				, mSeen(std::move(aRHS.mSeen))
			{
			}

			void ReserveAdditional(int aAdditions)
			{
				mMetadataPool.ReserveIncrease(aAdditions);
			}

			bool AlreadyProcessed(const SequenceUnit& aCandidate) const 
			{
				return mSeen.find(&aCandidate) != mSeen.end(); 
			}
			
			void SetProcessed(const SequenceUnit& aCandidate)
			{
				mSeen.insert(&aCandidate); 
			}

			void Reset()
			{
				mWorldStates.clear();

				// Reset will pre-clear any content in the pool which is a one time cost, but can be heavy
				// Clear will attenuate any metadata clearing to acquisition which spreads it over the solve
				mMetadataPool.Clear();
				mSeen.clear();
			}

			void RollBackUnit(SequenceUnit& aCandidate)
			{
				
			}

			ActionMetadataPool& GetMetadataPool() { return mMetadataPool; }
			WorldStateStack& GetWorldstates() { return mWorldStates; }

		private:
			using PathFlagData = std::unordered_set<const SequenceUnit*>;

			ActionMetadataPool mMetadataPool;
			WorldStateStack mWorldStates;

			PathFlagData mSeen;
		};
	}
}