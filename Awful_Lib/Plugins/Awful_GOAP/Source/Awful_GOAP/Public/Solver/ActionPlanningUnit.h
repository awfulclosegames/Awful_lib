// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once


#include "GraphOptimizer/GraphNode.h"
#include "ActionHandle.h"

namespace Awful
{
	class Condition;

	class ActionPlanningUnit : public GO::GraphNode<ActionHandle, ActionPlanningUnit>
	{
	public:
		using BASE = GO::GraphNode<ActionHandle, ActionPlanningUnit>;
		using ActionType = typename ActionHandle::ActionType;
		ActionPlanningUnit() = default;


		void Initialize(ActionType& aRef, float aCost)
		{
			mLocator.SetID(aRef.mID);
			mLocator.SetCost(aCost);

			// This s a hack way to forward references without a bunch of code
			mAction = &aRef;
		}

		bool operator==(const ActionPlanningUnit& aRHS) const
		{
			return mLocator.GetID() == aRHS.mLocator.GetID();
		}

		const ActionHandle& GetLocator() const { return mLocator; }

		float ComputeTraversalCost() const { return mLocator.GetCost(); }

		// Distance needs to know the context, and right now it's a bit abstract.
		// This should probably get rewritten to reference the current heuristic type  
		// as a metric of distance
		float ComputeDistanceToPoint(const ActionHandle& aPoint) const
		{
			if (aPoint.GetID() == mLocator.GetID())
				return 0.0f;

			return FLT_MAX;
		}

		const Condition& PreConditions() const { return mAction->mPrecondition; }
		const Condition& PostConditions() const { return mAction->mPostcondition; }

		const ActionType& GetAction() const { return *mAction; }

	private:
		friend class ActionLookup;

		ActionType* mAction = nullptr;
		ActionHandle mLocator;
	};

}
