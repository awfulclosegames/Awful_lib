// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>

#include "GraphOptimizer/BasicTypes.h"
#include "ActionMetadata.h"
#include "Solver/ActionPlanningUnit.h"
#include "Solver/WorldState.h"

namespace Awful
{
	class PlanningWorld;

	namespace ActionSpaceSearch
	{


		class FollowupFinder;

		class FollowupIteratorSupport
		{
			using NodeCollectionType = std::vector<ActionPlanningUnit>;

		public:
			using Edge = ActionPlanningUnit::Edge;
			using Action = ActionPlanningUnit::ActionType;

		private:
			friend class FollowupFinder;

			FollowupIteratorSupport(const NodeCollectionType& aNodeCollection, const ActionPlanningUnit& aNode, const ActionMetadata& aMD)
				: mActiveNode(aNode)
				, mNodeCollection(aNodeCollection)
				, mMetadata(aMD)
			{
			}

			NodeCollectionType::const_iterator RealEnd() { return mNodeCollection.end(); }
			
			const ActionPlanningUnit& mActiveNode;
			const NodeCollectionType& mNodeCollection;
			const ActionMetadata& mMetadata;
		};


		class FollowupFinder
		{
		public:
			using IteratorSupport = FollowupIteratorSupport;
			using ActionCollection = FollowupIteratorSupport::NodeCollectionType;

			FollowupIteratorSupport operator()(const ActionPlanningUnit& aNode, const ActionMetadata& aMD) const { return FollowupIteratorSupport(*mNodeCollection, aNode, aMD); }

		private:
			friend class PlanningWorld;
			ActionCollection* mNodeCollection = nullptr;

		};


	}
}