// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

// filter file to minimize the exposer of implementation details to calling code
#include "../private/BayesianNetwork/BayesianBeliefNetwork.h"
#include "../Private/BayesianNetwork/BucketElimination/ByesianInference_BucketElimination.h"

namespace Awful_BeliefNet
{
	using StandardInference = BucketElimination::BayesianInference_BucketElimination;
}