// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "GraphOptimizer/CoreGraphSolver.h"

#include "ActionSpaceSearch/ActionMetadata.h"
#include "ActionSpaceSearch/GOAP_StateExtension.h"
#include "ActionSpaceSearch/DynamicDependencyCache.h"

#include "Solver/CommonHeuristics.h"
#include "Solver/PlanningWorld.h"

// TODO I want to generalize the error reporting so I can get rid of these couts in the Unreal case and forward to the log

namespace Awful
{
	// Goal oriented action planner solver. Based on a graph sequence optimizer / path finder. Specifies the actions
	// as a search space (in the planning world) where each node is an action with a specific set of pre-conditions and results (post-conditions).
	// Then it searches for a viable sequence of actions that goes through the graph from start node to goal node. 
	// NOTE: this implementation does not create a fully specified graph, the edges are only implied by precondition acceptability 
	//       and are computed during the solve. This reduces the potential cost of the solve since edges are only computed on demand. 
	//       It also reduces the memory footprint and the impact of changing the action set. 
	//       
	//       A future improvement would be to cache edge evaluations since it is very likely a working set will emerge.


	// bit hard to read, templated base of a templated CRTP ... still not the craziest structure by a lot!
	template<class HeuristicClassType = NullGOAPHeuristic, class ActionSpaceWorld = PlanningWorld>
	class GOAP_Solver : public GO::GraphCoreSolver<ActionSpaceWorld, HeuristicClassType, GOAP_Solver<HeuristicClassType, ActionSpaceWorld>, ActionSpaceSearch::ActionMetadata, ActionSpaceSearch::GOAP_StateExtension>
	{

	private:
		using BASE = GO::GraphCoreSolver<ActionSpaceWorld, HeuristicClassType, GOAP_Solver<HeuristicClassType, ActionSpaceWorld>, ActionSpaceSearch::ActionMetadata, ActionSpaceSearch::GOAP_StateExtension>;

		using ReferencePointType = typename BASE::ReferencePointType;
		using ActionPlanningUnit = typename BASE::Node;
		using SequenceUnit = typename BASE::GraphSequenceUnit;
		using PlanningState = typename BASE::OptimizationState;
		using Action = typename ActionPlanningUnit::ActionType;

		using WorldstateAccessor = WorldStateStack::WorldStateAccessor;
	public:
		using HeuristicType = HeuristicClassType;
		using PlanningWorldType = ActionSpaceWorld;

		GOAP_Solver(const PlanningWorldType& aWorld);

		// annoyingly I get a type collision with the base class if I don't rename this :/
		// TODO: proper fix
		GO::JobToken InitiateGOAPJob(const Condition& aStartCondition, const ReferencePointType& aDest, HeuristicClassType& aHeuristic = sDefaultHeuristic, GO::GraphOptimizerConfig* config = nullptr);
		GO::JobToken InitiateGOAPJob(const Condition& aStartCondition, const Condition& aGoalCondition, HeuristicClassType& aHeuristic = sDefaultHeuristic, GO::GraphOptimizerConfig* config = nullptr);

	private:
		friend BASE;

		bool Validate(const ActionPlanningUnit& aCandidate,  const WorldstateAccessor& aWorldstate) const;
		bool ValidateImpact(const ActionPlanningUnit& aCandidate, const WorldstateAccessor& aWorldstate, Condition& prunedPostConditions) const;
		void Prune(const ActionSpaceSearch::ActionMetadata::KeySet& aValidatedActions, const WorldstateAccessor& aWorldstate, ActionSpaceSearch::ActionMetadata::KeySet& aValidateSet) const;


		void PushInitial(PlanningState& aState);
		bool PopLowest(PlanningState& aState);
		void AddCandidateNeighbours(PlanningState& aState);

		ActionSpaceSearch::DynamicDependencyCache mCache;
	};


	// ................................................................................
	//  Inlined methods
	// ................................................................................



	template<class HeuristicClassType, class ActionSpaceWorld>
	GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::GOAP_Solver(const PlanningWorldType& aWorld)
		: BASE(aWorld)
		, mCache(BASE::GetWorld())
	{
	}


		// TODO:
		// TODO:
		// TODO:
	template<class HeuristicClassType, class ActionSpaceWorld>
	GO::JobToken GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::InitiateGOAPJob(const Condition& aStartCondition,
		const typename BASE::ReferencePointType& aDest,
		HeuristicClassType& aHeuristic, typename GO::GraphOptimizerConfig* config/* = nullptr*/)
	{
		BASE::EnterCriticalSection();
		GO::JobToken token = BASE::GenerateToken();
		BASE::ExitCriticalSection();

		return token;
	}
	template<class HeuristicClassType, class ActionSpaceWorld>
	GO::JobToken GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::InitiateGOAPJob(const Condition& aStartCondition,
		const Condition& aGoalCondition,
		HeuristicClassType& aHeuristic, typename GO::GraphOptimizerConfig* config/* = nullptr*/)
	{
		BASE::EnterCriticalSection();
		GO::JobToken token = BASE::GenerateToken();
		BASE::ExitCriticalSection();

		return token;
	}


	template<class HeuristicClassType, class ActionSpaceWorld>
	void GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::PushInitial(PlanningState& aState)
	{
		const ActionPlanningUnit& initialAction = BASE::GetWorld().GetNode(aState.GetSource());
		const ActionPlanningUnit& goalAction = BASE::GetWorld().GetNode(aState.GetGoal());

		// Start with a zero cost, but that's just an assumption for now. Maybe this should be overridable?
		float initialHeuristicCost = 0.0f;
		float initialTraversalCost = initialAction.GetTraversalCost();

		SequenceUnit& seqUnit = aState.RegisterUnit(initialAction.GetInternalGUID(), initialAction.GetInternalGUID(), SequenceUnit::InvalidID, initialHeuristicCost, initialTraversalCost);
		seqUnit.Instantiate(aState.GetMetadataPool());
		seqUnit.SetExpectedWorld(aState.GetWorldstates().AcquireLayer());

		seqUnit.AddToExpectedWorld(initialAction.PostConditions());

		aState.AddCandidate(seqUnit);
		aState.SetInitial(seqUnit);

		// add any actions that are validated by the initial state of the world
		const auto& currentWorldState = seqUnit.GetExpectedWorld();
		auto& validActions = seqUnit.GetValidatedActions();

		for (auto& currentAction : BASE::GetWorld().Iterate())
		{
			// if they are valid we can add them to the known valid list to potentially expand as neighbours
			if (Validate(currentAction, currentWorldState))
			{
				validActions.insert(currentAction.GetInternalGUID());
			}
		}

#ifdef _DEBUG
		std::cout << " GOAL: " << goalAction.GetAction().mName << " : ";
		for (auto& cond : goalAction.GetAction().mPrecondition)
		{
			std::cout << cond.first << "(" << cond.second << ") ";
		}
		std::cout << std::endl << " ===================================" << std::endl;
#endif
	}

	template<class HeuristicClassType, class ActionSpaceWorld>
	bool GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::Validate(const ActionPlanningUnit& aCandidate, const WorldstateAccessor& aWorldstate) const
	{
		bool isValid = true;
		for (auto& candidateCondition : aCandidate.PreConditions())
		{
			bool conditionMatched = aWorldstate.contains(candidateCondition);
			isValid = isValid && conditionMatched;
		}
		return isValid;
	}

	template<class HeuristicClassType, class ActionSpaceWorld>
	bool GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::ValidateImpact(const ActionPlanningUnit& aCandidate, const WorldstateAccessor& aWorldstate, Condition& prunedPostConditions) const
	{
		bool isValid = false;
		for (auto& candidateCondition : aCandidate.PostConditions())
		{
			// technically we can stop at the first true, but the conditional in a loop 
			// is probably worse than the extra test?
			bool conditionValid = !aWorldstate.contains(candidateCondition);
			isValid |= conditionValid;
			if (conditionValid)
			{
				prunedPostConditions.Add(candidateCondition);
			}
		}
		return isValid;
	}


	template<class HeuristicClassType, class ActionSpaceWorld>
	void GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::Prune(const ActionSpaceSearch::ActionMetadata::KeySet& aValidatedActions, const WorldstateAccessor& aWorldstate, ActionSpaceSearch::ActionMetadata::KeySet& aValidateSet) const
	{
		for (const auto& key : aValidatedActions)
		{
			const auto& currentAction = BASE::GetWorld().GetNode(key);
			if (Validate(currentAction, aWorldstate))
			{
				aValidateSet.emplace(key);
			}
		}
	}

	// similar to the A* case, this feels like a loop in serious need of optimizations
	template<class HeuristicClassType, class ActionSpaceWorld>
	bool GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::PopLowest(PlanningState& aState)
	{
		bool found = false;

		while (!(found || aState.NoMoreCandidates()))
		{
			const SequenceUnit* cheapestUnit = aState.PopCandidate();

			// always return the destination even if it represents a world state we've seen
			found = (!aState.AlreadyProcessed(*cheapestUnit)) || aState.IsDestination(*cheapestUnit) || aState.NoMoreCandidates();
			aState.SetProcessed(*cheapestUnit);
		}

#ifdef _DEBUG
		if (found)
		{
			static int sEXPLORATION_COUNT = 0;
			auto& debugUnit = aState.GetActiveUnit();
			const ActionPlanningUnit& debugCheapest = BASE::GetWorld().GetNode(debugUnit.GetTargetNodeGUID());
			auto& world = debugUnit.GetExpectedWorld();
			std::cout << sEXPLORATION_COUNT++ << " |   " << debugCheapest.GetAction().mName << "  T: " << debugUnit.GetTraversalCost() << "  H: " << debugUnit.GetHeuristicCost() << " : ";
			for (auto& cond : world)
			{
				std::cout << cond.first << "(" << cond.second << ") ";
			}
			std::cout << std::endl;
		}
#endif
		return found;
	}

	template<class HeuristicClassType, class ActionSpaceWorld>
	void GOAP_Solver<HeuristicClassType, ActionSpaceWorld>::AddCandidateNeighbours(PlanningState& aState)
	{
		const auto& currentUnit = aState.GetActiveUnit();
		const float baseTraversal = currentUnit.GetTraversalCost();

		const auto& currentAction = BASE::GetWorld().GetNode(aState.GetCurrent());
		const auto& goalAction = BASE::GetWorld().GetNode(aState.GetGoal());
		// an optimization to avoid redundant searches through the current node's expected worldstate
		aState.GetWorldstates().EnableCaching(true);
		
		// given that each candidate/transition must be added (and potentially each route through the candidates) 
		// means that this list can get very long. It's simple for now, but it would be better to update a single entry or even
		// a stack. For now we want to keep this parallelizable. But when it's time to optimize, this is a good place to look!
		const auto& currentWorldState = currentUnit.GetExpectedWorld();

		// the edges (transitions between actions) are inferred by possible candidates in the action space
		int transitionIndex = -1;
		ActionSpaceSearch::ActionMetadata::KeySet validActions;
		{   // scoping the call to the currentWorldState, sicne we may disturb the memory it refernces
			// we will need to make sure we don't reuse it after this loop
			Prune(currentUnit.GetValidatedActions(), currentWorldState, validActions);

			const auto& dependencies = mCache(currentAction.GetInternalGUID());
			// this is all the other actions that can be affected by the postoncditions of the initial action
			for (const auto& currentDep : dependencies)
			{
				const bool isNewAction = validActions.find(currentDep) == validActions.end();
				
				// if they are valid we can add them to the known valid list to potentially expand as neighbours
				const auto& dependentNode = BASE::GetWorld().GetNode(currentDep);
				if (isNewAction && Validate(dependentNode, currentWorldState))
				{
					validActions.insert(currentDep);
				}
			}
		}
		aState.ReserveAdditional(static_cast<int>(validActions.size()));

		// loop through all possible followup actions from the current action in the current worldstate
		//   THIS should probably be parallelized, or else we should batch it on chains of evaluation 
		//   which would be changed in the core solver
		for (const auto& followup : validActions)
		{
			GO::PathGUID neighbourGuid = followup;
			const auto& neighbour = BASE::GetWorld().GetNode(neighbourGuid);
			Condition prunedPostConditions;
			if (!(aState.IsDestination(followup) || ValidateImpact(neighbour, currentWorldState, prunedPostConditions)))
			{
				// if it's not altering our worldstate, there isn't much reason to re-explore it unless it is the goal
				continue;
			}

			++transitionIndex;

			float heuristicCost = 0.0f;
			float traversalCost = baseTraversal + neighbour.GetTraversalCost();

			// a non-void return block for the updated values might be a better flow?
			aState.GetHeuristic()->ComputeStep(currentUnit, neighbour, goalAction, typename BASE::Edge(followup), heuristicCost);
 
			SequenceUnit& candidate = aState.RegisterUnit(currentAction.GetInternalGUID(), neighbour.GetInternalGUID(), transitionIndex, heuristicCost, traversalCost);
			candidate.Instantiate(aState.GetMetadataPool());

			candidate.SetExpectedWorld(currentWorldState);
			// normally we'd add the neighbour.PostConditions to the expected world with filtering, but since 
			// we just did the filtering search in the ValidateImpact() call we may as well save some time and just
			// explicitly add only the new conditions to the expected world
			// TODO: this shouldn't be done on add, no reason to pay for the layer etc if we never 
			// open this node so we should do this step as part of the pop_lowest or on entry with the popped item
			candidate.AddToExpectedWorldExplicit(prunedPostConditions);

			// it's always worth recording a step to the goal even if it's NOT the lowest cost or we've already pushed it
			if (!aState.AlreadyProcessed(candidate) || aState.IsDestination(neighbourGuid))
			{
				candidate.SetValidatedActions(validActions);

				// update how we got here so we can build a proper sequence if it comes to it
				candidate.SetPrevID(aState.GetActiveUnit().GetID());
				aState.AddCandidate(candidate);
			}
			else
			{
				// this worldstate has already been seen, roll it back and try again
				candidate.Deinstantiate();
				aState.RollBackUnit(candidate);
			}
		}
		// clear the caches and turn off caching till the next cycle
		aState.GetWorldstates().EnableCaching(false);
		aState.GetWorldstates().ResetCache();
	}
}