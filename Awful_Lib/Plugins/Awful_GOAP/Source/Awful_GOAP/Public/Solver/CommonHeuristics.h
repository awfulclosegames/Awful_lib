// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "GraphOptimizer/BasicTypes.h"
#include "GraphOptimizer/HeuristicBase.h"
#include "BaseAction.h"
#include "ActionPlanningUnit.h"
#include "CommonDataTypes/Condition.h"
#include "WorldState.h"
#include "ActionSpaceSearch/ActionMetadata.h"


namespace Awful
{

	// could derive from the base heuristic, but then I'd need to implement a virtual call... or I suppose
	// go and do yet another CRTP type structure here. Simpler for now just to interface by convention 

	template <class CostFunctor>
	class GenericGOAPHeurisitc
	{
	public:
		using Edge = typename ActionPlanningUnit::Edge;

		GenericGOAPHeurisitc()
		{
		}


		void ComputeStep(const ActionSpaceSearch::ActionMetadata& aCurrentAction, const ActionPlanningUnit& aNeighbourAction
			, const ActionPlanningUnit& aGoalAction, const Edge& aEdge
			, float& aHeuristicCost) const
		{
			aHeuristicCost = CostFunctor{}(aCurrentAction, aNeighbourAction, aGoalAction);
		}
	};


	// ................................................................................
	// now some common cost functions
	// ................................................................................

	//  Todo add:
	//	  Problem space growth factor -- How much it opens the problem space
	//    Inverse world state impact
	

	class NullHeuristic
	{
	public:
		float operator()(const ActionSpaceSearch::ActionMetadata&, const ActionPlanningUnit&, const ActionPlanningUnit& aGoalAction) const { return 0.0f; }
		float operator()(const Condition&, const ActionPlanningUnit&) const { return 0.0f; }

	};

	class MinimumWorldstateSize
	{
		using WorldStateAccessor = typename ActionSpaceSearch::ActionMetadata::WorldstateAccessor;
	public:
		float operator()(const ActionSpaceSearch::ActionMetadata& aFrom, const ActionPlanningUnit& aTo, const ActionPlanningUnit& aGoalAction) const
		{
			// we see how much we will add to the current world
			return Compute(aFrom.GetExpectedWorld(), aTo.PostConditions());
		}

	private:
		float Compute(const WorldStateAccessor& aWorld, const Condition& conditions) const
		{
			int count = 0;
			for (auto& cond : conditions)
			{
				count += !aWorld.contains(cond);
			}
			return static_cast<float>(count);
		}

	};


	class MaximumWorldstateImpact
	{
		using WorldStateAccessor = typename ActionSpaceSearch::ActionMetadata::WorldstateAccessor;
	public:
		// rethink how I manage config params
		float operator()(const ActionSpaceSearch::ActionMetadata& aFrom, const ActionPlanningUnit& aTo, const ActionPlanningUnit& aGoalAction) const
		{
			const bool isGoalStep = aTo.GetInternalGUID() == aGoalAction.GetInternalGUID();
			if (isGoalStep)
			{
				// kindof a hack, and can lead to aggressive early out with a goal in sight even if it's not
				// the optimum plan. But it should still be a nearly optimum plan, and the performance gain 
				// can be significant. 
				// TODO: look into a better implementation here
				return -1;
			}
			// we see how much we will add to the current world
			return Compute(aFrom.GetExpectedWorld(), aTo.PostConditions(), aGoalAction, isGoalStep);
		}

	private:
		float Compute(const WorldStateAccessor& aWorld, const Condition& conditions, const ActionPlanningUnit& aGoalAction, bool aIsGoalStep) const
		{
			float Penalty = !aIsGoalStep;
			float openCount = 0.0f;

			for (const auto& pre : aGoalAction.PreConditions())
			{
				Penalty += !aWorld.contains(pre);
			}

			// we see how much we will add to the current world, how many of these conditions
			// are not already in the world
			float count = 0;
			for (auto& cond : conditions)
			{
				count += !aWorld.contains(cond);
			}
			float currentSize = static_cast<float>(aWorld.size());
			float newSize = currentSize + count;

			float factor = (currentSize / newSize);
			
			// heuristically favour adding to the worldstate after it's quite large
			return (1.0f - factor) * Penalty;
		}
	};


	// ................................................................................
	// now define the usable heuristics
	// ................................................................................



	// assumption that the open prerequisites is a metric of how far from the goal we are. 
	// NOTE this distance is not linear
	class OpenPrereqsDist
	{
		using WorldStateAccessor = typename ActionSpaceSearch::ActionMetadata::WorldstateAccessor;
	public:
		using Edge = typename ActionPlanningUnit::Edge;
		OpenPrereqsDist()
		{}
		void ComputeStep(const ActionSpaceSearch::ActionMetadata& aCurrentAction, const ActionPlanningUnit& aNeighbourAction
			, const ActionPlanningUnit& aGoalAction, const Edge& aEdge
			, float& aHeuristicCost) const
		{
			auto world = aCurrentAction.GetExpectedWorld();
			float openCount = 0.0f;
			
			for (const auto& pre : aGoalAction.PreConditions())
			{ 
				if (world.contains(pre))
					continue;

				// would this transition add a worldstate item that is a current unmet precondition of the goal?
				bool found = aNeighbourAction.PostConditions().Contains(pre);
				// if not then we're adding things that don't directly improve our plan
				openCount += !found;
			}

			aHeuristicCost = openCount;
		}
	};


	// prefer smaller world states to minimize side effects
	using WorldStateGrowth = GenericGOAPHeurisitc<MinimumWorldstateSize>;

	// prefer rapidly expanding the world state to rapidly try and allow our goal
	using WorldStateImpact = GenericGOAPHeurisitc<MaximumWorldstateImpact>;

	using NullGOAPHeuristic = GenericGOAPHeurisitc<NullHeuristic>;

}
