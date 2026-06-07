// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "CommonDataTypes/Condition.h"
#include "Platform/PoolString.h"

namespace Awful
{
	class ContinuousCondition : public ConditionBase<PoolString, float>
	{
	};
}