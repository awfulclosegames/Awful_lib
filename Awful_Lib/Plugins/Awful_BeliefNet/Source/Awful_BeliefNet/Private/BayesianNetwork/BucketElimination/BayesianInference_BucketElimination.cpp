// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include <algorithm>

#include "BayesianNetwork/Internal/BucketElimination/BayesianInference_BucketElimination.h"


namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		// Depth first through the buckets to generate a topological order
		void BayesianInference_BucketElimination::SetupBuckets(const InferenceNode& aNode, Ordering& aOrdering)
		{
			VisitedList visited;

			SetupBucketsRecursive(aNode, aOrdering, visited);
			// start off with the root of this ordering as the significant parent
			BayesianBucket* initialSignificantParent = *aOrdering.begin();
			for (BayesianBucket* currentBucket : ReverseRange(aOrdering))
			{
				currentBucket->SetSignificantParent(*initialSignificantParent);
				unsigned int mask = 1;

				// go through all of my children and gather unique parents
				for (BayesianBucket* currentChild : currentBucket->GetChildren())
				{
					for (auto& childParent : currentChild->GetParents())
					{
						bool isNotCurrenetParent = currentBucket->GetKey() != childParent.first->GetKey();
						if (isNotCurrenetParent)
						{
							currentBucket->AddUniqueParent(*childParent.second.first, mask);
						}
					}
					mask <<= 1;
				}
				if (!currentBucket->GetParents().empty())
				{
					BayesianBucket& rightmostParent = FindRightmostParent(aOrdering, *currentBucket);
					currentBucket->SetSignificantParent(rightmostParent);
					rightmostParent.AddChild(*currentBucket);
				}
			}

		}

		// should refactor this, there are improvements to the loopyness of this all
		void BayesianInference_BucketElimination::SetupBucketsRecursive(const InferenceNode& aCurrent, Ordering& aOrdering, VisitedList& aVisited)
		{
			BayesianBucket& currentBucket = aOrdering.GetBucket(aCurrent.GetKey());

			auto& parentBuckets = currentBucket.GetParents();
			unsigned int mask = 1 << parentBuckets.size();

			// go through the conditioning cases of each node, as represented by the parents list of the bucket
			const BBN_Node& baseNode = aCurrent;
			for (auto& currentCase : baseNode.GetCPT())
			{
				BayesianBucket& parentBucket = aOrdering.GetBucket(currentCase.node->GetKey());

				// returns true if the insertion occurred and false if it was already there
				if (currentBucket.AddUniqueParent(parentBucket, mask))
				{
					SetupBucketsRecursive(parentBucket, aOrdering, aVisited);
				}
				mask <<= 1;
			}

			auto [itr, result] = aVisited.insert(aCurrent.GetKey());
			if (result)
			{
				// this wasn't already in the visited list, but is now
				// so add us to the ordering
				currentBucket.SetTopologicalIndex(aOrdering.size());
				aOrdering.push_back(&currentBucket);
				// and all our children
				for (auto& childHandle : GetChildren(currentBucket))
				{
					// TODO:
					// this should still be cleaned up. I 'know' that this won't, or shouldn't, get out of synch
					// with the BBN we're built around. But it would be nice to structure this in a way that doesn't let
					// it. Ideally without needing to do a lot of null and range checks inside these loops
					BBN_Node::NodeKeyType childKey = GetNode(childHandle).GetKey();
					SetupBucketsRecursive(mInferenceNodes[childKey], aOrdering, aVisited);
				}
			}
		}

		// trying to find the rightmost parent in the ordering
		// right and left are defined from the ordering, leftmost is begin rightmost is --end()
		BayesianBucket& BayesianInference_BucketElimination::FindRightmostParent(const Ordering& aOrdering, const BayesianBucket& aChild)
		{
			// if we have no parents than just the start of the ordering
			unsigned int rightmostIndex = 0;
			for (auto& currentParent : aChild.GetParents())
			{
				const BayesianBucket* currentParentBucket = currentParent.second.first;
				const unsigned int newIndex = currentParentBucket->GetTopologicalIndex();
				bool fartherRight = rightmostIndex < newIndex;
				unsigned int selector = static_cast<unsigned int>(fartherRight);
				// lerp trick to chose new option or stick with the old.
				// If the new option is not farther right, then the selector is 0 and 1 - 0 is 1 so we keep the existing index
				// and ignore the new one, otherwise we do the opposite and ignore the existing one and take the new one
				rightmostIndex = (rightmostIndex * (1 - selector)) + (newIndex * selector);
			}
			return aOrdering[rightmostIndex];
		}


		// TODO:
		// the single inference queries should check if the ordering exists, and if not add one before proceeding.
		float BayesianInference_BucketElimination::SingleQuery(const IdentifierType aID, const ProbabilitySet& aObservations)
		{
			NodeHandle handle = GetHandle(aID);
			if (handle.IsValid())
			{
				return SingleQuery(handle, aObservations);
			}
			return -1.0f;
		}

		float BayesianInference_BucketElimination::SingleQuery(const NodeHandle aHandle, const ProbabilitySet& aObservations)
		{
			Synchronize();

			MarkFromObservations(aObservations);

			const BBN_Node& node = GetNode(aHandle);
			BBN_Node::NodeKeyType key = node.GetKey();
			if (key >= 0 && key < mInferenceNodes.size())
			{
				InferenceNode& inferenceNode = mInferenceNodes[key];

				if (aObservations.empty() || (inferenceNode.GetOrderingIndex() == Ordering::InvalidIndex))
				{
					return node.GetPriorProbability();
				}

				// do real work if we are not just asking for the priors
				Ordering& ordering = mOrderings[inferenceNode.GetOrderingIndex()];
				return ComputeConfidence(ordering);
			}
			return -1.0f;
		}

		const BayesianInference_BucketElimination::ConfidenceResult BayesianInference_BucketElimination::MostConfidentQuery(const ProbabilitySet& aObservations, float aThreshold)
		{
			Synchronize();

			MarkFromObservations(aObservations);

			ConfidenceResult bestResult;
			for (auto& rootKey : mRoots)
			{
				InferenceNode& currentNode = mInferenceNodes[rootKey];
				Ordering& currentOrdering = mOrderings[currentNode.GetOrderingIndex()];
				float confidence = ComputeConfidence(currentOrdering);
				if (confidence > bestResult.Confidence)
				{
					bestResult.ID = currentNode.GetIdentifier();
					bestResult.Confidence = confidence;
				}
			}
			return bestResult;
		}

		void BayesianInference_BucketElimination::ConfidenceQuery(ResultListType& aResults, const ProbabilitySet& aObservations, float aThreshold)
		{
			Synchronize();

			MarkFromObservations(aObservations);

			for (auto& rootKey : mRoots)
			{

				InferenceNode& currentNode = mInferenceNodes[rootKey];
				Ordering& currentOrdering = mOrderings[currentNode.GetOrderingIndex()];
				float confidence = ComputeConfidence(currentOrdering);
				aResults.emplace_back(currentNode.GetIdentifier(), confidence);
			}
			ConfidenceResult::Comparator comparitor;
			std::sort(aResults.begin(), aResults.end(), comparitor);
		}



		// the heart of the matter right here
		float BayesianInference_BucketElimination::ComputeConfidence(Ordering& aOrdering)
		{
			// early out for the unlikely case that the ordering is empty
			if (aOrdering.size() == 0)
			{
				return -1.0f;
			}

			// also check if we can early out in the unlikely case that the query node is observed (so we don't need to compute it's probability)
			// the ordering is topological so our query node is the left most (lowest index)
			const InferenceNode& queryNode = aOrdering[0];
			if (queryNode.IsObserved())
			{
				return queryNode.GetObservedProbability();
			}

			//******************************************************************************************
			// propagate observation status up the chain, so we can ignore nodes that have nothing observed
			// below them
			PrepareOrdering(aOrdering);

			// TODO:
			// this needs a refactor. The results are accumulated in place so we just "know" where to look for them. This is bad
			BayesianBucket& inferenceBucket = aOrdering[0];
			if (!inferenceBucket.IsActive())
			{
				// None of these observations touch the inference target of this query, so we can skip the rest of the solve
				// and just provide the prior
				const NodeType& inferenceNode = inferenceBucket;
				return inferenceNode.GetPriorProbability();
			}

			//******************************************************************************************
			// process all the buckets in topological order
 			for (BayesianBucket* currentBucket : ReverseRange(aOrdering))
			{
				if (!currentBucket->IsActive())
				{
					// neither this bucket nor any of it's children are active, so we can skip since it means only the 
					// priors of this node and it's children are relevant
					continue;
				}

				currentBucket->ComputePermutation();

				// this is a crucial and heavy traffic part of the code. future optimizations here will have a big effect.
				// simple branching now for readability, but this is a prime candidate for refactoring 
				if (currentBucket->IsObserved())
				{
					const InferenceNode& node = *currentBucket;
					// we've directly observed this node and set it's probability accordingly depending on if we observed it as true or false
					// Later generalize better. For now we just quantize belief into true/false
					currentBucket->FactorByBelief(node.GetObservedProbability() > NodeType::sDefaultProbability);
				}
				else
				{
					currentBucket->Marginalize();
				}
				auto& significantParent = currentBucket->GetSignificantParent();
				significantParent.AddFactor(currentBucket->GetFactor());

				// we only do one high index to 0 through the buckets, so we can clear the mark flag as we go
				// so we don't need to do a separate pass to clear it for the next query 
				currentBucket->ClearMark();
			}


			float rawPosConfidence = inferenceBucket.GetPositiveBelief();
			float totalConfidence = inferenceBucket.GetTotalBelief();
			if (totalConfidence <= 0.0f)
			{
				// error case, this should not happen 
				// but if it does we don't want to return a confidence value that is not between 0 and 1, 
				// so just return -1 to indicate an error
				return -1.0f;
			}
			return rawPosConfidence / totalConfidence;
		}

		void BayesianInference_BucketElimination::MarkFromObservations(const ProbabilitySet& aObservations)
		{
			// clear any existing flags and then for any nodes that are in our observation set,
			// mark them as observed and set their observed probability to the value in the observation set
			for (auto& currentNode : mInferenceNodes)
			{
				currentNode.Reset();
				auto result = aObservations.Find(currentNode.GetIdentifier());
				if (aObservations.Valid(result))
				{
					currentNode.SetObserved(true);
					currentNode.SetObservedProbability(aObservations.GetValue(result));
				}
			}
		}

		void BayesianInference_BucketElimination::PrepareOrdering(Ordering& aOrdering)
		{			 
			// propagate the marked flag up the ordering so that we know which buckets are active and which are not.
			for (BayesianBucket* currentBucket : ReverseRange(aOrdering))
			{
				currentBucket->Reset();
				bool isMarked = currentBucket->IsMarked() || currentBucket->IsObserved();
				currentBucket->AddMark(isMarked);
				auto& parent = currentBucket->GetSignificantParent();
				parent.AddMark(isMarked);
			}
		}


		void BayesianInference_BucketElimination::Synchronize()
		{
			if (mRevisionNumber != GetRevisionNumber() || mInferenceNodes.empty())
			{
				mRevisionNumber = GetRevisionNumber();
				mOrderings.Clear();
				mInferenceNodes.clear();
				mRoots.clear();
				mInferenceNodes.reserve(GetNodes().size());
				unsigned int index = 0;
				for (auto& currentNodeHandle : GetNodes())
				{
					auto& node = GetNode(currentNodeHandle);
					mInferenceNodes.emplace_back(node);
					if (node.TestFlagValue(NodeType::ROOT))
					{
						mRoots.push_back(index);
					}
					++index;
				}

				mOrderings.Setup(mRoots, mInferenceNodes);
				for (auto rootIndex : mRoots)
				{
					auto& node = mInferenceNodes[rootIndex];
					Ordering& currentOrdering = mOrderings[node.GetOrderingIndex()];
					SetupBuckets(node, currentOrdering);
				}
			}
		}

	}
}