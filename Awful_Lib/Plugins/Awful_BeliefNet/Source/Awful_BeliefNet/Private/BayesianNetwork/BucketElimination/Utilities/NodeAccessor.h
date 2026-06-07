// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "BayesianNetwork/Utilities/BBN_Accessor.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		class NodeAccessor : private BBN_Accessor
		{
			using Super = BBN_Accessor;
		public:
			using NodeList = Super::NodeList;
			using NodeHandle = Super::NodeHandle;

			NodeAccessor(BayesianBeliefNetwork& aBBN)
				: Super(aBBN)
			{}

			const BBN_Node& GetNode(NodeHandle aHandle) const { return Super::GetNode(aHandle); }

		};
	}
}