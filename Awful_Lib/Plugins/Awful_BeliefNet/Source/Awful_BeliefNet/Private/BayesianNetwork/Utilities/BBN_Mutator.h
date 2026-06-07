// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "BayesianNetwork/BayesianBeliefNetwork.h"
#include "BayesianNetwork/Utilities/BBN_Accessor.h"

namespace Awful_BeliefNet
{
	// an attorney class for writing data not exposed. Good for editors and serializations
	// mutating is also accessing, so you get that for free. Technically this means you pay twice for the BBN 
	// reference, but honestly that is almost certainly never going to matter
	class BBN_Mutator : protected BBN_Accessor
	{
		using Super = BBN_Accessor;
	public:
		BBN_Mutator(BayesianBeliefNetwork& aBBN)
			: Super(aBBN)
			, mBBN(aBBN)
		{
		}
	protected:
		using NodeHandle = Super::NodeHandle;
		NodeHandle CreateRootNode(IdentifierType aID, float aPriorProb) { return mBBN.CreateRootNode(aID, aPriorProb); }
		NodeHandle CreateEvidenceNode(IdentifierType aID, float aPriorProb) { return mBBN.CreateEvidenceNode(aID, aPriorProb); }
		void AddConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability)
		{
			mBBN.AddConditionalProbability(aParent, aChild, aConditionalProbability);
		}

		void AddConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild, float aConditionalProbability)
		{
			mBBN.AddConditionalProbability(aParent, aChild, aConditionalProbability);
		}
		void ApplyPriors(const ProbabilitySet& aPriors) { mBBN.ApplyPriors(aPriors); }

		void RemoveNode(IdentifierType aID) { return mBBN.RemoveNode(aID); }

		void RemoveConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild)
		{
			mBBN.RemoveConditionalProbability(aParent, aChild);
		}

		void RemoveConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild)
		{
			mBBN.RemoveConditionalProbability(aParent, aChild);
		}

		void EditConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability)
		{
			mBBN.EditConditionalProbability(aParent, aChild, aConditionalProbability);
		}

	private:
		BayesianBeliefNetwork& mBBN;

	};

}