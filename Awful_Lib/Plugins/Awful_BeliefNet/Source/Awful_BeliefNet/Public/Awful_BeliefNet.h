// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

// filter file to minimize the exposer of implementation details to calling code
#include "BayesianNetwork/Internal/BayesianBeliefNetwork.h"
#include "BayesianNetwork/Internal/BucketElimination/BayesianInference_BucketElimination.h"

namespace Awful_BeliefNet
{
	using StandardInference = BucketElimination::BayesianInference_BucketElimination;
}