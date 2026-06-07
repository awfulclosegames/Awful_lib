// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include <vector>
#include <set>
#include <string>
#include <unordered_map>
#include <functional>

#include "GraphOptimizer/GraphWorld.h"

#include "ActionSpaceSearch/ActionLookup.h"
#include "ActionSpaceSearch/FollowupFinder.h"
#include "CommonHeuristics.h"

namespace Awful
{
	class ActionPlanningUnit;
	class BaseAction;


	// NOTE:
	//   ActionLookup is a utility class to find a given action's representation in the graph (the node for that action)
	//       the node is a wrapper to identify the action for graph operations and to get the action back once we have a 
	//       sequence of nodes, which is a sequence of actions
	//   FollowupFinder is taking the place of the edge finder. In a spatial sequencing, this would evaluate the links
	//       between regions, but in for planning we want to find viable next actions (or the nodes that represent them)
	class PlanningWorld : public GO::GraphWorld<ActionPlanningUnit, ActionSpaceSearch::ActionLookup, ActionSpaceSearch::FollowupFinder>
	{
	public:
		using BASE = GO::GraphWorld<ActionPlanningUnit, ActionSpaceSearch::ActionLookup, ActionSpaceSearch::FollowupFinder>;
		using WorldDataType = typename BASE::WorldDataType;
		static int sInvalidRevision;


		PlanningWorld()
			: BASE()
		{
			GetEdgeFinder().mNodeCollection = &GetNodes();
		}

		// TODO
		//   should this signature be base action or relative to the action lookup class?
		//   both the action planning unit and the base action are obtainable from the lookup
		ActionPlanningUnit& AddAction(BaseAction& aRef, float aCost);

		// Remove an action and invalidate the dependency cache
		void RemoveAction(ActionPlanningUnit& aAction);

		const WorldDataType& GetAllActions() const;
		int GetRevision() const { return mRevision; }
	private:
		int mRevision = 0;
	};
}
