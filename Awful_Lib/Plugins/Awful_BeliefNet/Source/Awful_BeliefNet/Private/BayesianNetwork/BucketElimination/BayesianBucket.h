// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <map>

#include "Utilities/PackedIndexRemapping.h"

#include "BayesianNetwork/BucketElimination/ConditioningFactor.h"
#include "BayesianNetwork/BucketElimination/InferenceNode.h"

namespace Awful_BeliefNet
{

	namespace BucketElimination
	{
		// mostly a unit of state/relationships for the bucket elimination solve. It allows us
		// to not put implementation specifics in the node, which itself allows for an indirection
		// linking the BBN functionality and the domain that we're using the probabilities to reason
		// about
		// 
		// These handle most of the bucket elimination specific computations around conditioning and marginalization, 
		// but they are not responsible for the actual graph manipulation of the bucket elimination algorithm.
		// That is still the responsibility of the inference provider, which can access the internal state of the
		// buckets as needed to perform those operations.
		// 
		// The result is that the buckets are now significantly larger, so this should be monitored
		// 
		// TODO: 
		// the buckets are referencing each other directly, probably better to use an index or some similar thing
		class BayesianBucket
		{
		public:
			using ParentBuckets = std::map<const BayesianBucket*, std::pair<BayesianBucket*, unsigned int>>;
			using BucketList = std::vector<BayesianBucket*>;
			using IndexType = InferenceNode::IndexType;
			using KeyList = InferenceNode::KeyList;

			static constexpr unsigned int INVALID_INDEX = static_cast<unsigned int>(-1);

			BayesianBucket(InferenceNode& aReference);
			BayesianBucket(BayesianBucket&& aRHS);

			// for convenience, let us cast to the reference node
			operator const InferenceNode& () { return mReference; }
			operator const BBN_Node& () const { return mReference; }

			bool HasParent(const BayesianBucket& aTestParent) const;

			void SetSignificantParent(BayesianBucket& aParent) { mSignificantParent = &aParent; }
			BayesianBucket& GetSignificantParent() const { return *mSignificantParent; }

			void FactorBelief() { mFactor.ComputeBeliefFilter(); }
			void FactorDisbelief() { mFactor.ComputeDisbeliefFilter(); }
			void Marginalize() { mFactor.ComputeBeliefSums(); }
			const ConditioningFactor& GetFactor() const { return mFactor; }

			bool IsActive() const;
			bool IsObserved() const;

			// attorney pattern? accessor class? just a friend with the inference class?
			BucketList& GetChildren() { return mChildren; }
			ParentBuckets& GetParents() { return mParents; }
			const ParentBuckets& GetParents() const { return mParents; }

			bool AddUniqueParent(BayesianBucket& aNewParent, unsigned int aMask);
			void AddChild(BayesianBucket& aNewChild);

			float GetPositiveBelief() const;
			float GetTotalBelief() const;
			
			typename BBN_Node::NodeKeyType GetKey() const { return mReference.GetKey(); }

			void SetTopologicalIndex(unsigned int aIndex) { mTopologicalIndex = aIndex; }
			unsigned int GetTopologicalIndex() const { return mTopologicalIndex; }

			void AddFactor(const BucketElimination::ConditioningFactor& aFactor) { mInheritedFactors.push_back(&aFactor); }

			bool IsMarked() const { return mMarked; }
			void AddMark(bool aValue) { mMarked |= aValue; }
			void ClearMark() { mMarked = false; }

			void ComputePermutation();

			void Reset();
			void Clear();
		private:
			using RemapTable = Awful::PackedIndexRemapping<SequenceKey>;
			using FactorList = std::vector<const ConditioningFactor*>;


			void ComputeIndexMappings(KeyList& aAccumulatedIndexes);
			IndexType RemapToBits(const IndexType aFrom, unsigned int aRemap) const;
			IndexType RemapReverseLocalIndex(IndexType aFrom) const;

			IndexType RemapIndexes(const IndexType aFrom, unsigned int aRemapIdx) const;
			IndexType RemapToLocalIndex(const IndexType aFrom) const;

			float AccumulateFactors(const IndexType aIndex) const;


			const InferenceNode& mReference;
			ConditioningFactor mFactor;

			BucketList mChildren;
			ParentBuckets mParents;
			// store a key here instead?
			BayesianBucket* mSignificantParent;

			RemapTable mIndexRemapping;
			FactorList mInheritedFactors;

			unsigned int mTopologicalIndex = INVALID_INDEX;
			bool mMarked = false;
		};

	}
}