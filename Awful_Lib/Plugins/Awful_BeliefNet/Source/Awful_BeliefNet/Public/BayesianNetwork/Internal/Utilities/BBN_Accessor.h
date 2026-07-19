// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "BBN_API.h"

#include "BayesianNetwork/Internal/BayesianBeliefNetwork.h"

namespace Awful_BeliefNet
{
	// an attorney class for accessing data not exposed. Specifically for editor and inference implementations

	class AWFUL_BBN_API BBN_Accessor
	{
	public:
		BBN_Accessor(BayesianBeliefNetwork& aBBN)
			: mBBN(aBBN)
		{}
	protected:
		using NodeType = BayesianBeliefNetwork::NodeType;
		using NodeList = BayesianBeliefNetwork::NodeList;
		using NodeHandle = BayesianBeliefNetwork::NodeHandle;
		using VisitedList = BayesianBeliefNetwork::VisitedList;


		const NodeList& GetRoots() const { return mBBN.GetRoots(); }
		const NodeList& GetNodes() const { return mBBN.GetNodes(); }
		const NodeList& GetChildren(const NodeType& aNode) const { return mBBN.GetChildren(aNode); }

		const NodeHandle GetHandle(const IdentifierType aID) const { return mBBN.GetHandle(aID); }
		const BBN_Node& GetNode(const NodeHandle aKey) const { return mBBN.GetNode(aKey); }

		void ReadPriors(ProbabilitySet& aPriors) const { mBBN.ReadPriors(aPriors); }

		unsigned int GetRevisionNumber() const { return mBBN.mRevisionNumber; }
	private:
		const BayesianBeliefNetwork& mBBN;

	};

}