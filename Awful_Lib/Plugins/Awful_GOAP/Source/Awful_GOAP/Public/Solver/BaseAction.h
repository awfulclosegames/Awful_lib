// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "Platform/PoolString.h"
#include "CommonDataTypes/Condition.h"


namespace Awful
{
	class BaseAction
	{
	public:
		using ActionID = unsigned int;

		//void Reset() { mExpectedWorld.Reset(); }

		int cost = 1;


		// hack to directly access these. 
		// Fix once this is running

		ActionID mID = -1;

		PoolString mName;

		Condition mPrecondition;
		Condition mPostcondition;
	};

}
