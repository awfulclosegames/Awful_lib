// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "BayesianNetwork/Utilities/BBN_Mutator.h"

namespace Awful_BeliefNet
{
	// for reading and writing priors and conditional probabilities 
	// NOTE: these operations do not alter the structure of the network 
	// so can be done cheaply without triggering a regeneration of inference data
	class ProbabilityEditor : private BBN_Mutator
	{
		using SUPER = BBN_Mutator;
	public:
		ProbabilityEditor(BayesianBeliefNetwork& aBBN)
			: SUPER(aBBN)
		{
		}
		void ApplyPriors(const ProbabilitySet& aPriors) { SUPER::ApplyPriors(aPriors); }
		void ReadPriors(ProbabilitySet& aPriors) const { SUPER::ReadPriors(aPriors); }
		// edit will only alter an existing conditional probability it will not add a new one
		void EditConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild, float aConditionalProbability)
		{
			SUPER::EditConditionalProbability(aParent, aChild, aConditionalProbability);
		}

	private:

	};
}