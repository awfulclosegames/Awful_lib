// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "BayesianNetwork/Internal/BucketElimination/InferenceNode.h"

#include "BayesianNetwork/Internal/BBN_Node.h"
#include "BayesianNetwork/Internal/ConditionalProbabilityTable.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		InferenceNode::InferenceNode(const BBN_Node& aReference)
			: mReference(aReference)
			, mObservedProbability(0.0f)
			, mObserved(false)
		{
			for (const auto& currentCase : mReference.GetCPT())
			{
				mConditioningKeys.push_back(currentCase.node->GetKey());
			}
		}

		float InferenceNode::ComputeRow(const IndexType aIndex) const
		{
			return static_cast<const BBN_Node&>(*this).GetCPT().ComputeRow(aIndex);
		}

		void InferenceNode::Reset()
		{
			mObserved = false;
		}

	}
}