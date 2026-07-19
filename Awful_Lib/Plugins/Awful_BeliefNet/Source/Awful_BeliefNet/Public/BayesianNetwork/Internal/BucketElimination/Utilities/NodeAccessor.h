// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "BBN_API.h"

#include "BayesianNetwork/Internal/Utilities/BBN_Accessor.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		class AWFUL_BBN_API NodeAccessor : private BBN_Accessor
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