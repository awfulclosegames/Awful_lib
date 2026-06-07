// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "Platform/PoolString.h"
#include "CommonDataTypes/ContinuousConditionr.h"

namespace Awful_BeliefNet
{
	// for associating nodes with external data, also a human readable ID for nodes
	using IdentifierType = Awful::PoolString;

	using ProbabilitySet = Awful::ContinuousCondition;

	using SequenceKey = unsigned int;

}
