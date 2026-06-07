// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "GraphOptimizer/BasicTypes.h"
#include "PathfindStateExtension.h"

#include "GraphOptimizer/CoreGraphSolver.h"
#include "GraphOptimizer/HeuristicBase.h"

namespace Awful
{
	namespace GO
	{

		template<class PathingWorld, class HeuristicClass = HeuristicBase<typename PathingWorld::Node> >
		class AStarPathFinder : public GraphCoreSolver<PathingWorld, HeuristicClass, AStarPathFinder<PathingWorld, HeuristicClass>, PathfindStateExtension::BaseType, PathfindStateExtension>
		{
		public:
			using BASE = GraphCoreSolver<PathingWorld, HeuristicClass, AStarPathFinder<PathingWorld, HeuristicClass>, PathfindStateExtension::BaseType, PathfindStateExtension>;
			
			AStarPathFinder()
				: BASE()
			{
			}
			AStarPathFinder(const PathingWorld& aWorld)
				: BASE(aWorld)
			{
			}
			
			AStarPathFinder(const AStarPathFinder& aRHS)
				: BASE(aRHS)
			{
			}
			
			AStarPathFinder(const AStarPathFinder&& aRHS)
				: BASE(std::move(aRHS))
			{
			}

		private:
			friend BASE;

			using PathfindingState = typename BASE::OptimizationState;
			using Region = typename BASE::Node;

			using ReferencePointType = typename BASE::ReferencePointType;
			using Region = typename BASE::Node;
			using PathUnit = typename BASE::GraphSequenceUnit;

			void PushInitial(PathfindingState& aState);
			bool PopLowest(PathfindingState& aState);
			void AddCandidateNeighbours(PathfindingState& aState);
		};

		template<class PathingWorld, class HeuristicClass>
		inline void AStarPathFinder<PathingWorld, HeuristicClass>::PushInitial(PathfindingState& aState)
		{
			const Region& initialRegion = BASE::GetWorld().GetNode(aState.GetSource());
			const Region& destRegion = BASE::GetWorld().GetNode(aState.GetDest());

			float initialHeuristicCost = 0.0f;

			// zero cost since we're already in this region, potentially want to compute partial traversal costs?
			PathUnit& pathUnit = aState.RegisterUnit(initialRegion.GetInternalGUID(), initialRegion.GetInternalGUID(), PathUnit::InvalidID, initialHeuristicCost, 0.0f);

			aState.AddCandidate(pathUnit);
			aState.SetInitial(pathUnit);
		}

		// needs optimization in a pretty severe way
		template<class PathingWorld, class HeuristicClass>
		inline bool AStarPathFinder<PathingWorld, HeuristicClass>::PopLowest(PathfindingState& aState)
		{
			bool done = aState.NoMoreCandidates();

			while (!done)
			{
				if (BASE::CheckForTimeout(aState))
				{
					std::cout << "Timed out in PopLowest!" << std::endl;
					return false;
				}

				const PathUnit* cheapestUnit = aState.PopCandidate();

				if (cheapestUnit == nullptr)
				{
					return false;
				}

#ifdef _DEBUG
				aState.DebugUnit = cheapestUnit;
#endif

				BASE::GetWorld().ProcessPoppedNode(cheapestUnit->GetSourceNodeGUID(), cheapestUnit->GetTargetNodeGUID());

				done = (!aState.AlreadyProcessed(*cheapestUnit)) || aState.IsDestination(*cheapestUnit) || aState.NoMoreCandidates();
				aState.SetProcessed(*cheapestUnit);
			}

			return true;
		}

		template<class PathingWorld, class HeuristicClass>
		inline void AStarPathFinder<PathingWorld, HeuristicClass>::AddCandidateNeighbours(PathfindingState& aState)
		{
			const PathUnit& currentUnit = aState.GetActiveUnit();
			float traversalCost = currentUnit.GetTraversalCost();

			const Region& currentRegion = BASE::GetWorld().GetNode(aState.GetCurrent());
			const Region& destRegion = BASE::GetWorld().GetNode(aState.GetDest());

			const float baseTraversal = traversalCost;

			// perform an A* over an arbitrarily connected graph.
			//   Nodes represent spatial regions (with a region traversal cost)
			//   Edges represent links or connections between regions (with a traversal cost)
			//
			// So regions a set of rooms and corridors (regions) connected by doors (links)
			// or a set of star systems (regions) connected by hyperspace gates (links)

			// should look into a better structure for the edges to reduce list clutter
			int linkIndex = -1;
			for (const auto& link : BASE::GetWorld().GetEdgess(currentRegion))
			{
				++linkIndex;

				PathGUID linkTarget = link.GetEdgeTarget();
				if (!aState.AlreadyProcessed(linkTarget))
				{
					const Region& neighbour = BASE::GetWorld().GetNode(linkTarget);

					float heuristicCost = 0.0f;
					float traversal = baseTraversal + link.GetTraversalCost() + neighbour.GetTraversalCost();

					aState.GetHeuristic()->ComputeMove(currentRegion, neighbour, destRegion, link, heuristicCost);

					PathUnit& neighbourUnit = aState.RegisterUnit(currentRegion.GetGUID(), neighbour.GetGUID(), linkIndex, heuristicCost, traversal);
					
					// track how a neighbour got there from us
					neighbourUnit.SetPrevID(aState.GetActiveUnit().GetID());
					aState.AddCandidate(neighbourUnit);
				}
			}
		}


	}
}