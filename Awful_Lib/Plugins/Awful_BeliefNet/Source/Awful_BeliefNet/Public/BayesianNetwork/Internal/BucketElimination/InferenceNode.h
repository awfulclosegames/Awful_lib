// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>

#include "BBN_API.h"

#include "BayesianNetwork/Internal/CommonTypes.h"
#include "BayesianNetwork/Internal/BBN_Node.h"
#include "BayesianNetwork/Internal/ConditionalProbabilityTable.h"

namespace Awful_BeliefNet
{
	class BBN_Node;

	namespace BucketElimination
	{
		class AWFUL_BBN_API InferenceNode
		{
		public:
			using KeyList = std::vector<SequenceKey>;
			using IndexType = ConditionalProbabilityTable::IndexType;
			using NodeListType = std::vector<InferenceNode>;

			
			InferenceNode(const BBN_Node& aReference);

			InferenceNode(InferenceNode&& aRHS) noexcept
				: mReference(aRHS.mReference)
				, mConditioningKeys(std::move(aRHS.mConditioningKeys))
				, mObservedProbability(aRHS.mObservedProbability)
				, mObserved(aRHS.mObserved)
			{}

			BBN_Node::NodeKeyType GetKey() const { return mReference.GetKey(); }
			IdentifierType GetIdentifier() const { return mReference.GetIdentifier(); }

			const KeyList& GetConditioningKeys() const { return mConditioningKeys; }

			operator const BBN_Node&() const { return mReference; }

			float GetObservedProbability() const { return mObservedProbability; }
			void SetObservedProbability(float aValue) { mObservedProbability = aValue; }

			float ComputeRow(const IndexType aIndex) const;

			bool IsObserved() const { return mObserved; }
			void SetObserved(bool aValue) { mObserved = aValue; }

			void SetOrderingIndex(int aIndex) { mOrderingIndex = aIndex; }
			unsigned int GetOrderingIndex() const { return mOrderingIndex; }

			void Reset();

		private:

			const BBN_Node& mReference;

			KeyList mConditioningKeys;
			unsigned int mOrderingIndex = static_cast<unsigned int>(-1);

			float mObservedProbability = 0.0f;
			bool mObserved = false;
		};

	}
}