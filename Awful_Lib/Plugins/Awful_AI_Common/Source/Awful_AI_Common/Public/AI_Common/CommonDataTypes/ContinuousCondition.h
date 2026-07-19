// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "AI_Common/CommonDataTypes/Condition.h"
#include "AI_Common/Platform/PoolString.h"

namespace Awful
{
	class ContinuousCondition : public ConditionBase<PoolString, float>
	{
	};
}