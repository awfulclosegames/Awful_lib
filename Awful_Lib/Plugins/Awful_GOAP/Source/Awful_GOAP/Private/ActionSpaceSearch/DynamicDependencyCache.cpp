// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "ActionSpaceSearch/DynamicDependencyCache.h"
#include "Solver/PlanningWorld.h"
#include "Solver/BaseAction.h"

namespace Awful
{
	namespace ActionSpaceSearch
	{
		// TODO: right now the entire cache is invalidated when the world revision changes, but we reall
		// only need to invalidate specific action entires when they are modified. Tracking revisions on
		// individual actions would allow us to only invalidate the relevant cache entries instead of the 
		// entire cache, which would be a big performance boost for large worlds with localized changes.

	// start invalid so we build a cache on demand
	DynamicDependencyCacheImplementation::DynamicDependencyCacheImplementation(const PlanningWorld& aWorld)
		: mWorld(aWorld)
		, mRevision(PlanningWorld::sInvalidRevision)
	{}


	DynamicDependencyCacheImplementation::DynamicDependencyCacheImplementation(DynamicDependencyCacheImplementation&& aRHS)
		: mWorld(std::move(aRHS.mWorld))
		, mRevision(aRHS.mRevision)
#ifdef AwfulGOAP_ReversDependencyCache
		, mReverseDependencyMap(std::move(aRHS.mReverseDependencyMap))
#endif
		, mForwardDependencyMap(std::move(aRHS.mForwardDependencyMap))
	{}

	void DynamicDependencyCacheImplementation::Invalidate()
	{
		mRevision = PlanningWorld::sInvalidRevision;
		mForwardDependencyMap.clear();
#ifdef AwfulGOAP_ReversDependencyCache
		mReverseDependencyMap.clear();
#endif
	}

	// presumably this is not dirtying the world so just recompute the values for this action
	// for now we invalidate the whole cache, since we don't know that we are marking the action dirty
	// before or after it was changed. If after, than we can't trust the action preconditions for going 
	// through and removing it from the reverse mappings. If before then we don't know how to adjust the 
	// reverse mappings. So either way we currently wipe everything if anything changes. Additional bookkeeping 
	// (see the top of the file) around individual actions would help
	void DynamicDependencyCacheImplementation::MarkActionDirty(GO::PathGUID aNodeKey)
	{
		Invalidate();
	}

#ifdef AwfulGOAP_ReversDependencyCache
	// spin through all actions once (not great) to give us a fast look up for the on-demand dependency lookups
	// previously we would need to spin through all actions at least once per new dependency query, so this 
	// should be a huge win 
	void DynamicDependencyCacheImplementation::RebuildPreconDependencyMap()
	{
		// Invalidate all cached dependency results since the map changed
		Invalidate();

		// Clear existing reverse dependency map
		mReverseDependencyMap.clear();

		// Build new reverse dependency map
		// For each action's precondition, add the action to the appropriate list
		for (const auto& nodeEntry : mWorld.GetAllActions())
		{
			const auto& node = nodeEntry.GetAction();
			// For each precondition in this action
			for (const auto& precond : node.mPrecondition)
			{
				// Add this action to the list of actions that require this precondition
				mReverseDependencyMap[precond.GetKey()].push_back(nodeEntry.GetInternalGUID());
			}
		}
		mRevision = mWorld.GetRevision();
	}
#endif

	// TODO: see top of file for a discussion of how this can be optimized by tracking revisions on individual actions instead of the entire world
	const DynamicDependencyCacheImplementation::DependentNodes& DynamicDependencyCacheImplementation::GetDependencies(GO::PathGUID aNodeKey)
	{
		// Check if cache is stale
		if (mRevision != mWorld.GetRevision())
		{
#ifdef AwfulGOAP_ReversDependencyCache
			// Cache needs to be rebuilt
			RebuildPreconDependencyMap();
#else
			Invalidate();
			mRevision = mWorld.GetRevision();

#endif
		}

		// Try to find in cache
		auto cached = mForwardDependencyMap.find(aNodeKey);
		if (cached != mForwardDependencyMap.end())
		{
			return cached->second;
		}

		// Not in cache, compute now
		return FindNodeDependencies(aNodeKey);
	}


#ifdef AwfulGOAP_ReversDependencyCache
	const DynamicDependencyCacheImplementation::DependentNodes& DynamicDependencyCacheImplementation::FindNodeDependencies(GO::PathGUID aNodeKey)
	{
		auto& node = mWorld.GetNode(aNodeKey);

		// Build the list of dependent actions
		// An action depends on this action if any of its preconditions match any of this action's postconditions
		DependentNodes& dependencies = mForwardDependencyMap[aNodeKey];
		dependencies.clear();

		const auto& currentPostConditions = node.GetAction().mPostcondition;

		// set instead of vector since we want to keep entries unique
		std::unordered_set<GO::PathGUID> candidateDependencies;

		for (const auto& postCondition : currentPostConditions)
		{
			const auto dependsOnCondition = mReverseDependencyMap.find(postCondition.GetKey());
			if (dependsOnCondition != mReverseDependencyMap.end())
			{
				for (const auto candidateGuid : dependsOnCondition->second)
				{
					candidateDependencies.insert(candidateGuid);
				}
			}
		}
		dependencies.reserve(candidateDependencies.size());
		for (const auto candidateGuid : candidateDependencies)
		{
			dependencies.push_back(candidateGuid);
		}

		return dependencies;
	}
#else
	const DynamicDependencyCacheImplementation::DependentNodes& DynamicDependencyCacheImplementation::FindNodeDependencies(GO::PathGUID aNodeKey)
	{
		auto& node = mWorld.GetNode(aNodeKey);

		// Build the list of dependent actions
		// An action depends on this action if any of its preconditions match any of this action's postconditions
		DependentNodes& dependencies = mForwardDependencyMap[aNodeKey];
		dependencies.clear();

		const auto& currentPostConditions = node.GetAction().mPostcondition;
		for (const auto& candidate : mWorld.GetAllActions())
		{
			for (const auto& preconObservation : candidate.GetAction().mPrecondition)
			{
				const bool isDependent = currentPostConditions.ContainsKey(preconObservation);

				if (isDependent)
				{
					dependencies.push_back(candidate.GetInternalGUID());
					break;
				}
			}
		}

		return dependencies;
	}
#endif


	DynamicDependencyCache::DynamicDependencyCache(const PlanningWorld& aWorld)
		: DynamicDependencyCacheImplementation(aWorld)
	{}

	}
}