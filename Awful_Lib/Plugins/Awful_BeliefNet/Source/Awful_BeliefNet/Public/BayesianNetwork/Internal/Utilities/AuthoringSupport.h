// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "BBN_API.h"

#include "BayesianNetwork/Internal/Utilities/BBN_Mutator.h"

namespace Awful_BeliefNet
{
	class AWFUL_BBN_API AuthoringSupport : private BBN_Mutator
	{
		using SUPER = BBN_Mutator;
	public:
		AuthoringSupport(BayesianBeliefNetwork& aBBN)
			: SUPER(aBBN)
		{}

		NodeHandle CreateRootNode(IdentifierType aID, float aPriorProb) { return SUPER::CreateRootNode(aID, aPriorProb); }
		NodeHandle CreateEvidenceNode(IdentifierType aID, float aPriorProb) { return SUPER::CreateEvidenceNode(aID, aPriorProb); }
		void AddConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability)
		{
			SUPER::AddConditionalProbability(aParent, aChild, aConditionalProbability);
		}
		void AddConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild, float aConditionalProbability)
		{
			SUPER::AddConditionalProbability(aParent, aChild, aConditionalProbability);
		}

		void RemoveNode(IdentifierType aID) { return SUPER::RemoveNode(aID); }
		void RenameNode(IdentifierType aOldID, IdentifierType aNewID) { return SUPER::RenameNode(aOldID, aNewID); }

		void RemoveConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild)
		{
			SUPER::RemoveConditionalProbability(aParent, aChild);
		}
		void RemoveConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild)
		{
			SUPER::RemoveConditionalProbability(aParent, aChild);
		}
	private:

	};
}