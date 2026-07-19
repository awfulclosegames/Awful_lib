// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include <cmath>

#include "BayesianNetwork/Internal/ConditionalProbabilityTable.h"
#include "BayesianNetwork/Internal/BucketElimination/ConditioningFactor.h"
#include "BayesianNetwork/Internal/BBN_Node.h"

namespace Awful_BeliefNet
{
	ConditionalProbabilityTable::ConditionalProbabilityTable(ConditionalProbabilityTable&& aRHS) noexcept
		: mOwner(aRHS.mOwner)
		, mConditioningCases(std::move(aRHS.mConditioningCases))
	{
	}
	ConditionalProbabilityTable::ConditionalProbabilityTable(BBN_Node& aNode)
		: mOwner(aNode)
	{
		void Reset();
	}

	void ConditionalProbabilityTable::Add(BBN_Node* aNode, float aCondition)
	{
		mConditioningCases.push_back({aNode, aCondition});
	}

	void ConditionalProbabilityTable::Remove(BBN_Node* aNode)
	{ 
		for (auto currentCase = mConditioningCases.begin(); currentCase != mConditioningCases.end(); ++currentCase)
		{
			if (currentCase->node == aNode)
			{
				mConditioningCases.erase(currentCase);
				break;
			}
		}
	}


	// not expected to be frequent, nor are we expecting these lists to get long. So simple linear search for now.
	// If this becomes an issue we can implement a proper lookup
	void ConditionalProbabilityTable::Adjust(BBN_Node* aNode, float aCondition)
	{
		for (auto& currentCase : mConditioningCases)
		{	
			if (currentCase.node == aNode)
			{
				currentCase.probability = aCondition;
				break;
			}
		}
	}


	// compute a noisy-or style permutation of the conditioning table
	// ideally this would allow for sparce specifications but that can be added later.
	// this is a basic branchless implementation, that still uses floating point arithmetic
	// but it might be faster to implement with bitwise operations. Although this is a more old 
	// school hack, and relies on knowing the implementation of the floating point operations
	float ConditionalProbabilityTable::ComputeRow(const ConditionalProbabilityTable::IndexType aIndex) const
	{
		float prior = mOwner.GetPriorProbability();
		float result = prior;
		float accumulated = 1.0f;
		unsigned int bitSelector = 0;
		// TODO: this is quite regular code and could be vectorized nicely
		for (auto& currentCase : mConditioningCases)
		{
			// the logic here is to accumulate active probabilities
			// such that if the current bit is unset we accumulate the probability
			// and if it is set we accumulate 1 - the probability
			unsigned int mask = static_cast<unsigned int>(aIndex[bitSelector]);
			float selector = static_cast<float>(mask);
			// mask is 0 or 1, use lerp to select between the two cases
			float AdjustedProb = std::lerp(0.0f, currentCase.probability, selector);

			accumulated *= 1.0f - AdjustedProb;
			result = std::lerp(result, 1.0f - accumulated, selector);
			++bitSelector;
		}
		return result;
	}


	void ConditionalProbabilityTable::Clear()
	{
		mConditioningCases.clear();
	}

}
