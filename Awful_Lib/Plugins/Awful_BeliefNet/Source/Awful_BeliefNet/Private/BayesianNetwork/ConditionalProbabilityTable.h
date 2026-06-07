// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <bitset>
#include <vector>

#include "BayesianNetwork/CommonTypes.h"
#include "BayesianNetwork/ConditionalProbabilityTable.h"

namespace Awful_BeliefNet
{
	class BBN_Node;

	// messy forward declaration within a sub-namespace
	namespace BucketElimination
	{
		class ConditioningFactor;
	}

	// a simple conditioning table implementation for our simple bucket elimination implementation
	// single relations with positive probability and negative probability summing to 1 (so we can 
	// have the table only contain the positive probability and infer the negative as we're computing
	class ConditionalProbabilityTable
	{
	private:
		// just a value pair, but it's nice to have named parameters
		struct ConditioningCase
		{
			BBN_Node* node = nullptr;
			float probability = 0.5f;
		};

		using ConditionCaseList = std::vector<ConditioningCase>;

	public:
		using IndexType = std::bitset<32>;

		// limited indexes for remapping. Given the n^2 operations here we do not want more than 32
		// entries in a conditioning table right now. Maybe look at generalizing this when we implement
		// an MCMC solver
		using ConditioningCaseType = ConditioningCase;
		using Const_Iterator = ConditionCaseList::const_iterator;

		ConditionalProbabilityTable(ConditionalProbabilityTable&& other) noexcept;
		ConditionalProbabilityTable(BBN_Node& aLocal);

		void Add(BBN_Node* aNode, float aCondition);
		void Remove(BBN_Node* aNode);
		void Adjust(BBN_Node* aNode, float aCondition);

		Const_Iterator begin() const { return mConditioningCases.begin(); }
		Const_Iterator end() const { return mConditioningCases.end(); }
		size_t Size() const { return mConditioningCases.size(); }

		float ComputeRow(const IndexType aIndex) const;
		const ConditioningCase& operator[](unsigned int aIndex) const { return mConditioningCases[aIndex]; }

		void Clear();
	private:
		BBN_Node& mOwner;
		ConditionCaseList mConditioningCases;

	};

}