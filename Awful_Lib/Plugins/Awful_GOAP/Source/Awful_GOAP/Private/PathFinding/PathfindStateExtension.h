// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <set>

#include "GraphOptimizer/BasicTypes.h"
#include "GraphOptimizer/SequenceUnit.h"




namespace Awful
{
	namespace GO
	{

		class PathfindStateExtension
		{
		public:

			using PathUnit = SequenceUnit<>;

			PathfindStateExtension()
				: mSeen()
			{
			}
			PathfindStateExtension(const PathfindStateExtension& aRHS)
				: mSeen(aRHS.mSeen)
			{
			}
			PathfindStateExtension(const PathfindStateExtension&& aRHS)
				: mSeen(std::move(aRHS.mSeen))
			{
			}
			bool AlreadyProcessed(const PathUnit& aCandidate) const { return AlreadyProcessed(aCandidate.GetTargetNodeGUID()); }
			bool AlreadyProcessed(PathGUID aCandidate) const { return mSeen.find(aCandidate) != mSeen.end(); }
			void SetProcessed(const PathUnit& aCandidate) { mSeen.insert(aCandidate.GetTargetNodeGUID()); }

			void Reset() { mSeen.clear(); }

#ifdef _DEBUG
			// to keep a reference of the cheapest for testing and debugging
			PathUnit const* DebugUnit = nullptr;
#endif

		private:
			using PathFlags = std::set<unsigned int>;

			PathFlags mSeen;
		};
	}
}