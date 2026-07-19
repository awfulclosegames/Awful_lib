// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "BBN_API.h"

#include "BayesianNetwork/Internal/CommonTypes.h"
#include "BayesianNetwork/Internal/ConditionalProbabilityTable.h"


namespace Awful_BeliefNet
{
	class NodeStoreType;
	class BayesianBeliefNetwork;

	class AWFUL_BBN_API BBN_Node
	{
	public:
		using NodeHandle = BBN_Node*;
		using NodeKeyType = SequenceKey;
		// coin toss if we don't actually know
		static constexpr const float sDefaultProbability = 0.5f;

		// a bit flag to describe the state of the node
		enum NodeStateFlags
		{
			NONE = 0
			, ROOT = 1
			, EVIDENCE = 2
			, NEGATED = 4
		};


		BBN_Node(IdentifierType aID, float aPrior)
			: mCPT(*this)
			, mPriorProbability(aPrior)
			, mIdentifier(aID)
		{
		}


		IdentifierType GetIdentifier() const { return mIdentifier; }
		NodeKeyType GetKey() const { return mKey; }

		void SetPriorProbability(float aValue) { mPriorProbability = aValue; }
		float GetPriorProbability() const;

		bool TestFlagValue(BBN_Node::NodeStateFlags aValue) const { return TestFlagValue(static_cast<unsigned int>(aValue)); }
		bool TestFlagValue(unsigned int aValue) const { return (mCurrentState & aValue) == aValue; }
		void SetFlagValue(BBN_Node::NodeStateFlags aValue) { SetFlagValue(static_cast<unsigned int>(aValue)); }
		void SetFlagValue(unsigned int aValue) { mCurrentState |= aValue; }
		void ClearFlagValue(BBN_Node::NodeStateFlags aValue) { ClearFlagValue(static_cast<unsigned int>(aValue)); }
		void ClearFlagValue(unsigned int aValue) { mCurrentState &= ~aValue; }
		void FlipFlagValue(BBN_Node::NodeStateFlags aValue) { FlipFlagValue(static_cast<unsigned int>(aValue)); }
		void FlipFlagValue(unsigned int aValue) { mCurrentState ^= aValue; }

		unsigned int ReadFlag(unsigned int aValue) const { return mCurrentState & aValue; }

		const ConditionalProbabilityTable::ConditioningCaseType& GetConditioningCases(unsigned int aIndex) const { return mCPT[aIndex]; }
		void AddConditionalProbability(NodeHandle aCondition, float aProbability);
		void RemoveConditionalProbability(NodeHandle aCondition);
		void ChangeConditionalProbability(NodeHandle aCondition, float aProbability);

		const ConditionalProbabilityTable& GetCPT() const { return mCPT; }

	protected:
		friend class NodeStoreType;
		friend class BayesianBeliefNetwork;
		friend class ConditionalProbabilityTable;

		
		BBN_Node()
			: mCPT(*this)
		{
		}

		void Clear();

		void SetIdentifier(IdentifierType aID) { mIdentifier = aID; }
		void SetKey(NodeKeyType aKey) { mKey = aKey; }
		ConditionalProbabilityTable& GetConditionalProbabilityTable() { return mCPT; }


	private:
		static const NodeKeyType InvalidKey = static_cast<NodeKeyType>(-1);

		BBN_Node(const BBN_Node& aRHS) = delete;
		const BBN_Node& operator= (const BBN_Node& aRHS) = delete;

		unsigned int mCurrentState = NodeStateFlags::NONE;

		ConditionalProbabilityTable mCPT;

		float mPriorProbability = sDefaultProbability;
		// used for internal reference withing the network
		NodeKeyType mKey = InvalidKey;
		IdentifierType mIdentifier;
	};
}