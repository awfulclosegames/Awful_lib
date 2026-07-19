// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <assert.h>

#include "BBN_API.h"

#include "BayesianNetwork/Internal/BucketElimination/InferenceNode.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		class BayesianBucket;

		// the factor is really an API for selectively filtering the positive or negative belief probabilities
		// and summing them to get a factor value. This is a CPT factor in the way that it allows the bucket
		// chains to build up by weighted product, a conditioning case.
		// The three 'modes' for the two filtering operations, and then finally summing them up only really are different
		// from each other at factor creation time (as we are doing the bucket walk) so use a functor template
		// on the constructor to do just the parts we need
		class AWFUL_BBN_API ConditioningFactor
		{
		public:
			using KeyList = InferenceNode::KeyList;
			using IndexType = InferenceNode::IndexType;

			inline float operator[](IndexType aIndex) const
			{
				unsigned int offset = ConvertToOffset(aIndex);
				return mRunningResults[offset + mOutputOffset];
			}
			const KeyList& GetKeys() const { return mKeys; }

		private:
			friend class BayesianBucket;
			using RunningResultsType = std::vector<float>;

			ConditioningFactor()
				: mKeys()
				, mRunningResults()
			{
			}

			ConditioningFactor(ConditioningFactor& aRHS) noexcept
				: mKeys(aRHS.mKeys)
				, mRunningResults(aRHS.mRunningResults)
				, mOutputOffset(aRHS.mOutputOffset)
			{			
				// we don't want to allow this, but for DLL linkage it throws a compile error not to have it
				assert(false);
			}


			ConditioningFactor(ConditioningFactor&& aRHS) noexcept
				: mKeys(std::move(aRHS.mKeys))
				, mRunningResults(std::move(aRHS.mRunningResults))
				, mOutputOffset(aRHS.mOutputOffset)
			{
			}

			KeyList& EditKeys() { return mKeys; }

			void SetRunningValue(IndexType aIndex, float aValue)
			{
				unsigned int offset = ConvertToOffset(aIndex);
				mRunningResults[offset] = aValue;
			}

			// cast the bool to either 0 or 1 and shift to find the appropriate location
			void FactorByBelief(bool aIsBelieved) { mOutputOffset = aIsBelieved << GetKeys().size(); }
			inline void ComputeBeliefSums();

			// these make use of the fact that we accumulate the total belief at the top of the running results, 
			// the positive belief at the offset, as a residue of the filtering and summing
			float GetPositiveBelief() const
			{
				unsigned int positiveOffset = 1 << GetKeys().size();
				return mRunningResults[positiveOffset];

			}

			//Too much implicit knowledge, like that the resultant total belief will wind up in the zero index
			float GetTotalBelief() const { return mRunningResults[0]; }

			void Resize(unsigned int aSize) { mRunningResults.resize(aSize); }

			void Reset() 
			{
				mKeys.clear();
				mRunningResults.clear();
				mOutputOffset = 0;
			}

			inline unsigned int ConvertToOffset(const IndexType aIndex) const { return aIndex.to_ulong(); }

			KeyList mKeys;	
			RunningResultsType mRunningResults;
			unsigned int mOutputOffset = 0;
		};


		void ConditioningFactor::ComputeBeliefSums()
		{
			// the partition is the point between the probabilities we computed for the case where we believe false
			// (the first half) and believe true (the second half) 
			// overwriting the table data is a poor implementation decision. 
			unsigned int partitionOffset = 1 << GetKeys().size();
			for (unsigned int i = 0; i < partitionOffset; ++i)
			{
				mRunningResults[i] += mRunningResults[i + partitionOffset];
			}
		}
	}
}