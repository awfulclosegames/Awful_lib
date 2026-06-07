// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <unordered_map>

#include "GraphOptimizer/BasicTypes.h"
#include "CommonDataTypes/Observable.h"


// the reverse dependency map should improve things, but benchmarks show it costing more than the 
// forward map alone
#undef AwfulGOAP_ReversDependencyCache
//#define AwfulGOAP_ReversDependencyCache

namespace Awful
{
	class PlanningWorld;

	namespace ActionSpaceSearch
	{
		class DynamicDependencyCacheImplementation
		{
		public:
			using DependentNodes = std::vector<GO::PathGUID>;
			using DependencyMapType = std::unordered_map<GO::PathGUID, DependentNodes>;
#ifdef AwfulGOAP_ReversDependencyCache
			using PreconditionDependencyMapType = std::unordered_map<Observable::KeyType, DependentNodes>;
#endif
			// Get actions that can be executed after the given action
			// This returns actions whose preconditions are satisfied by the given action's postconditions
			const DependentNodes& GetDependencies(GO::PathGUID aNodeKey);

			// Invalidate cache when actions are modified or removed
			void Invalidate();

			// Mark actions as dirty when their postconditions change
			void MarkActionDirty(GO::PathGUID aNodeKey);

		private:
			friend class DynamicDependencyCache;

			DynamicDependencyCacheImplementation(DynamicDependencyCacheImplementation&& aRHS);
			DynamicDependencyCacheImplementation(const PlanningWorld& aWorld);

#ifdef AwfulGOAP_ReversDependencyCache
			// Build/rebuild the reverse dependency map (postcondition -> list of actions)
			void RebuildPreconDependencyMap();
#endif
			// Given a node key, find all actions that depend on it
			const DependentNodes& FindNodeDependencies(GO::PathGUID aNodeKey);

			const PlanningWorld& mWorld;
			int mRevision = 0;  // Tracks whether cache needs rebuilding

#ifdef AwfulGOAP_ReversDependencyCache
			// Reverse dependency map: Given an observable key, which actions have it as a postcondition
			PreconditionDependencyMapType mReverseDependencyMap;
#endif
			// Cache of dependency results for each node
			DependencyMapType mForwardDependencyMap;
		};


		// attorney pattern FTW... ok a bit over-engineered for this use.
		// I wanted to prevent the general instantiation of these things but if I
		// made the constructor private I would need to make the solver a friend
		// which would defeat the purpose of encapsulating this class
		class DynamicDependencyCache : private DynamicDependencyCacheImplementation
		{
			using BASE = DynamicDependencyCacheImplementation;
		public:
			using DependentNodes = BASE::DependentNodes;

			const DependentNodes& operator()(GO::PathGUID aNode)
			{
				return BASE::GetDependencies(aNode);
			}

		private:
			template<class A, class B>
			friend class GOAP_Solver;

			DynamicDependencyCache(const PlanningWorld& aWorld);
		};


	}
}
