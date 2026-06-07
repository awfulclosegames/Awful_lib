// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "Solver/BaseAction.h"

namespace Awful
{
	class ActionHandle
	{
	public:
		using ActionType = BaseAction;
		using ID_Type = ActionType::ActionID;


		ActionHandle() = default;

		ActionHandle(ID_Type anId)
			: myID(anId)
		{
		}

		void SetID(ID_Type anId) { myID = anId; }
		ID_Type GetID() const { return myID; }
			
		// consider distance for action space
	
		float GetCost() const { return myTraversalCost; }
		void SetCost(float aCost) { myTraversalCost = aCost; }

	private:
		ID_Type myID = -1;
		float myTraversalCost = 0.0f;

	};
}