// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "Solver/PlanningWorld.h"
#include "Solver/BaseAction.h"

namespace Awful
{

	int PlanningWorld::sInvalidRevision = -1;


	ActionPlanningUnit& PlanningWorld::AddAction(BaseAction& aRef, float aCost)
	{
		auto& newAction = BASE::AddNode();
		newAction.Initialize(aRef, aCost);
		++mRevision;
		return newAction;
	}

	void PlanningWorld::RemoveAction(ActionPlanningUnit& aAction)
	{
		// Find the action in the world data
		auto it = std::find(GetNodes().begin(), GetNodes().end(), aAction);
		if (it != GetNodes().end())
		{
			// Remove the action from the vector
			GetNodes().erase(it);
			// Invalidate the revision counter to force cache rebuild
			++mRevision;
		}
	}

	const PlanningWorld::WorldDataType& PlanningWorld::GetAllActions() const
	{
		return BASE::GetNodes();
	}

}

