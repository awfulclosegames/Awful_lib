// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "BayesianNetwork/BucketElimination/Ordering.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		void Ordering::clear()
		{
			mOrder.clear();
			mBuckets.clear();
		}

		void Ordering::Fill(Ordering::NodeList& aNodes)
		{
			clear();
			mOrder.reserve(aNodes.size());
			mBuckets.reserve(aNodes.size());
			// create a parallel array of buckets with the same key/indexing of the nodes, so we can
			// reuse the node's key as a bucket index
			for (auto& current : aNodes)
			{
				mBuckets.emplace_back(BayesianBucket(current));
			}
		}




		// TODO: 
		// fill these on demand, or better yet note that the initial setup for
		// all orderings is the same
		void OrderingList::Setup(const KeyList& aRoots, NodeList& aNodes)
		{
			mOrderings.resize(aRoots.size());

			for (unsigned int idx = 0; idx < aRoots.size(); ++idx)
			{
				auto& currentRoot = aNodes[aRoots[idx]];
				currentRoot.SetOrderingIndex(idx);
				mOrderings[idx].Fill(aNodes);
			}
		}

	}
}