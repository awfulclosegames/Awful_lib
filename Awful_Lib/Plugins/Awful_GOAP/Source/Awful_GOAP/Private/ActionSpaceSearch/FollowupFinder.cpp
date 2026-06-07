// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "ActionSpaceSearch/FollowupFinder.h"
#include "Solver/PlanningWorld.h"

namespace Awful
{
	namespace ActionSpaceSearch
	{
	//	FollowupIteratorSupport::Edge FollowupIteratorSupport::EdgeIterator::operator*()
	//	{
	//		return Edge(mCurrent->GetInternalGUID());
	//	}

	//	//FollowupIteratorSupport::EdgeIterator& FollowupIteratorSupport::EdgeIterator::operator++()
	//	//{
	//	//	bool done = false;
	//	//	while (!done)
	//	//	{
	//	//		done = true;
	//	//		mCurrent++;
	//	//		if (mCurrent != mOwner.RealEnd())
	//	//		{
	//	//			// this is tree based so we don't really need  to bother with the test against
	//	//			// ancestors. If we care we can check in the metadata
	//	//			//	
	//	//			// test the accumulated worldstates of our active region against the prerequeistis of 
	//	//			// the candidate to see if it is satisfied
	//	//			// if so we can exit the loop because this action is viable
	//	//			done = Validate(mCurrent->GetAction());
	//	//		}
	//	//	}
	//	//	return *this;
	//	//}

	//	bool FollowupIteratorSupport::EdgeIterator::Validate(const Action& aCandidate)
	//	{
	//		return false;
	//		//const auto& expected = mOwner.GetExpectedWorldState();

	//		//bool isValid = true;
	//		//for (auto& candidateCondition : aCandidate.mPrecondition)
	//		//{
	//		//	// Scary loop, but this find() is on a map. So it's not as bad as it looks
	//		//	const auto worldCondition = expected.find(candidateCondition);

	//		//	bool conditionMatched = ((worldCondition != expected.end()) && (candidateCondition.GetValue() == worldCondition->second));

	//		//	isValid = isValid && conditionMatched;
	//		//}
	//		//return isValid;
	//	}

	//	FollowupIteratorSupport::EdgeIterator FollowupIteratorSupport::begin()
	//	{
	//		return FollowupIteratorSupport::EdgeIterator(mNodeCollection.begin(), *this);
	//	}

	//	FollowupIteratorSupport::EdgeIterator FollowupIteratorSupport::end()
	//	{
	//		return FollowupIteratorSupport::EdgeIterator(mNodeCollection.end(), *this);
	//	}
	}
}