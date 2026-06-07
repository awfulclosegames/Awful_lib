// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include <cmath>

#include "BayesianNetwork/CommonTypes.h"
#include "BayesianNetwork/BBN_Node.h"

namespace Awful_BeliefNet
{
	float BBN_Node::GetPriorProbability() const
	{
		// uses the bit flags to handle negation without branches
		// 1 if negated 0 if not negated
		float selector = static_cast<float>(TestFlagValue(NodeStateFlags::NEGATED));
		return std::lerp(mPriorProbability, 1.0f - mPriorProbability, selector);
	}


	void BBN_Node::AddConditionalProbability(NodeHandle aCondition, float aProbability)
	{
		mCPT.Add(aCondition, aProbability);
	}

	void BBN_Node::RemoveConditionalProbability(NodeHandle aCondition)
	{
		mCPT.Remove(aCondition);
	}

	void BBN_Node::ChangeConditionalProbability(NodeHandle aCondition, float aProbability)
	{
		mCPT.Adjust(aCondition, aProbability);
	}

	void BBN_Node::Clear()
	{
		mCPT.Clear();
		mPriorProbability = sDefaultProbability; 
		mCurrentState = NodeStateFlags::NONE;
		mKey = InvalidKey;
		mIdentifier = IdentifierType();
	}
}