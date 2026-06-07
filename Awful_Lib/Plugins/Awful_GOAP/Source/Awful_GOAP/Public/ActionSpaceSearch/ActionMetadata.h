// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <unordered_set>

#include "Utilities/Pooling/BasicPool.h"
#include "GraphOptimizer/BasicTypes.h"
#include "Solver/WorldState.h"
#include "Solver/WorldStateStack.h"

#include "Utilities/Pooling/ChunkPool.h"
// The Action Metadata is used to extend the sequence unit with
// goap/action specific data

namespace Awful
{
	template<class A, class B>
	class GOAP_Solver;

	namespace ActionSpaceSearch
	{
		class ActionMetadataValues 
		{
		public:
			using KeySet = std::unordered_set<GO::PathGUID>;

			const WorldStateStack::WorldStateAccessor& GetExpectedWorld() const { return mExpectedWorld; }
			// this should be const and 
			KeySet& GetValidatedActions() { return mValidActions; }

			// only intended to be used by pooling, but I suppose there's nothing invalid about
			// clearing this. Also, given the generic nature of the pools it is difficult to 
			// implement this without either a virtual call or another CRTP structure which feels
			// overengineered in this case
			void Clear()
			{
				mExpectedWorld.Reset();
				mValidActions.clear();
			}

		private:
			friend class ActionMetadata;
			//friend class StandardPool<ActionMetadataValues>;
			WorldStateStack::WorldStateAccessor mExpectedWorld;
			KeySet mValidActions;
		};

		//using ActionMetadataPool = StandardPool<ActionMetadataValues>;
		using ActionMetadataPool = ChunkedPool<ActionMetadataValues, 256>;

		class ActionMetadata
		{
		public:
			using KeySet = ActionMetadataValues::KeySet;
			using WorldstateAccessor = WorldStateStack::WorldStateAccessor;
			const WorldstateAccessor& GetExpectedWorld() { return GetStorage().GetExpectedWorld(); }

			const WorldstateAccessor& GetExpectedWorld() const { return GetStorage().GetExpectedWorld(); }

			const KeySet& GetValidatedActions() const { return GetStorage().GetValidatedActions(); }

			// this is a hack to avoid copies. Though I suspect a move operator would address this 
			// the  bigger issue is repeated (inner loop hits to GetStorage() and the indirections 
			KeySet& GetValidatedActions() { return GetStorage().GetValidatedActions(); }

			void ReserveExpectedWorld(int aSize)
			{
			}

			void SetExpectedWorld(const WorldstateAccessor& aOther)
			{
				GetStorage().mExpectedWorld = aOther;
			}

			void SetValidatedActions(const KeySet& aOther)
			{
				auto& validActions = GetStorage().mValidActions;
				validActions = aOther;
			}
			
			void AddToExpectedWorld(const Observable& aObs)
			{
				GetStorage().mExpectedWorld.insert(aObs);
			}

			// the explicit version requires that the input condition is pruned against the 
			// worldstate to only include new observations
			void AddToExpectedWorldExplicit(const Condition& aOther);
			void AddToExpectedWorld(const Condition& aOther);

			const ActionMetadataValues& ReadStorage() const { return GetStorage(); }

			bool IsInstantiated() const { return mStorageID.IsValid(); }
			
			std::size_t GetHash() const { return mHash; }
		private:

			ActionMetadataValues& GetStorage() const { return (*mPool)[mStorageID]; }

			// don't love the friend here, but it's simpler for now
			template<class A, class B>
			friend class Awful::GOAP_Solver;

			void Instantiate(ActionMetadataPool& aPool);
			void Deinstantiate();

			void Reset();

			ActionMetadataPool::ItemHandle mStorageID;
			ActionMetadataPool* mPool = nullptr;
			std::size_t mHash = 0;
		};

	}
}