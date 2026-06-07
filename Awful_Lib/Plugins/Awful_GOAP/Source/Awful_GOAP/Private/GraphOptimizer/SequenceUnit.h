#pragma once

#include "BasicTypes.h"
// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include <deque>

// defines a step in a sequence of nodes and edges that traverses a given graph

namespace Awful
{
	namespace GO
	{
		// it feels clumsy having both source and dest in here. It's just to support the edge index lookups and
		// it feels like there's a better way. Revisit this

		template <class BASE = DefaultSequenceBase>
		class SequenceUnit : public BASE
		{
		public:
			using BaseType = BASE;

			SequenceUnit(const PathGUID& aFromNodeID, const PathGUID& aToNodeID, int aEdgeIndex, float aHeuristicScore, float aTraversalCost)
				: BASE()
				, mSourceNode(aFromNodeID)
				, mTargetNode(aToNodeID)
				, mID(InvalidID)
				, mPrev(InvalidID)
				, mEdgeID(aEdgeIndex)
				, mHeuristicCost(aHeuristicScore)
				, mTraversalCost(aTraversalCost)
			{
			}

			PathGUID GetSourceNodeGUID() const { return mSourceNode; }
			PathGUID GetTargetNodeGUID() const { return mTargetNode; }
			float GetHeuristicCost() const { return mHeuristicCost; }
			float GetTraversalCost() const { return mTraversalCost; }
			float GetTotalCost() const { return mHeuristicCost + mTraversalCost; }

			void SetID(int aNewId) { mID = aNewId; }
			int GetID() const { return mID; }

			void SetPrevID(int aNewId) { mPrev = aNewId; }
			int GetPrev() const { return mPrev; }

			void SetEdgeID(int aNewId) { mEdgeID = aNewId; }
			int GetEdgeID() const { return mEdgeID; }

			// should probably typedef path unit IDs in case we need to make them more complex?
			static const int InvalidID = -1;

			using SequenceStore = std::deque<SequenceUnit>;

		private:
			PathGUID mSourceNode;
			PathGUID mTargetNode;
			// could consider maintaining a reference to the actual edge here?
			// or the nodes? 
			// That would mean giving up on the decoupling, right now there is nothing
			// binding a sequence step to a specific node/edge implementation
			int mID;
			int mPrev;
			int mEdgeID;
			float mHeuristicCost;
			float mTraversalCost;
		};
	}
}
