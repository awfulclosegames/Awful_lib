#pragma once
#include "BayesianNetwork/CommonTypes.h"

#include "BayesianNetwork/Utilities/BBN_Accessor.h"
#include "BayesianNetwork/BucketElimination/Utilities/NodeAccessor.h"
#include "BayesianNetwork/BucketElimination/Ordering.h"
#include "BayesianNetwork/BucketElimination/InferenceNode.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		class BayesianInference_BucketElimination : private BBN_Accessor
		{
			using SUPER = BBN_Accessor;
		public:
			using NodeHandle = SUPER::NodeHandle;
			struct ConfidenceResult
			{
				struct Comparitor
				{
					bool operator()(const ConfidenceResult& a, const ConfidenceResult& b)const { return a.Confidence > b.Confidence; }
				};

				IdentifierType ID;
				float Confidence = 0.0f;
			};
			using ResultListType = std::vector<ConfidenceResult>;

			BayesianInference_BucketElimination(BayesianBeliefNetwork& aBBN)
				: SUPER(aBBN)
				, mNodeAccessor(aBBN)
				, mOrderings()
				, mInferenceNodes()
				, mRoots()
			{
				Synchronize();
			}


			//*********************************************************************
			// Inference API
			// possibly also want a set of these that use Condition instead of ProbabilitySet
			// may save some busy work when connecting BBNs to other systems
			float SingleQuery(const IdentifierType aID, const ProbabilitySet& aObservations);
			float SingleQuery(const NodeHandle aHandle, const ProbabilitySet& aObservations);
			const ConfidenceResult MostConfidentQuery(const ProbabilitySet& aObservations, float aThreshold = 0.0f);
			void ConfidenceQuery(ResultListType& aResults, const ProbabilitySet& aObservations, float aThreshold = 0.0f);
			//*********************************************************************

		private:
			using VisitedList = SUPER::VisitedList;

			// quick and dirty trick to use range for syntax. Should add some extra stuff here to ensure that 
			// T is a valid container with proper iterator semantics
			template <typename T>
			struct ReverseRange
			{
				auto begin() { return reversedContainer.rbegin(); }
				auto end() { return reversedContainer.rend(); }
				T& reversedContainer;
			};


			BayesianInference_BucketElimination(BayesianInference_BucketElimination& aRHS) = delete;
			BayesianInference_BucketElimination(BayesianInference_BucketElimination&& aRHS) = delete;
			BayesianInference_BucketElimination& operator=(BayesianInference_BucketElimination& aRHS) = delete;

			float ComputeConfidence(Ordering& aOrdering);

			void MarkFromObservations(const ProbabilitySet& aObservations);
			void PrepareOrdering(Ordering& aOrdering);


			void SetupBuckets(const InferenceNode& aNode, Ordering& aOrdering);
			void SetupBucketsRecursive(const InferenceNode& aCurrent, Ordering& aOrdering, VisitedList& aVisited);
			BayesianBucket& FindRightmostParent(const Ordering& aOrdering, const BayesianBucket& aChild);

			void Synchronize();

			// this lets other things interpret node handles without exposing their inners. 
			// at some later point we could replace the internal references in the handles with 
			// something more elaborate
			NodeAccessor mNodeAccessor;
			OrderingList mOrderings;

			// Store our local set of nodes that have additional data for the bucket elimination algorithm
			// This part is still a bit sticky, to make this work easily, I need to keep it parallel with the nodes 
			// list in the BBN. So that BBN node sequence keys will work here. This was one advantage to using the handles
			// directly
			InferenceNode::NodeListType mInferenceNodes;
			InferenceNode::KeyList mRoots;

			unsigned int mRevisionNumber = 0;
		};
	}
}