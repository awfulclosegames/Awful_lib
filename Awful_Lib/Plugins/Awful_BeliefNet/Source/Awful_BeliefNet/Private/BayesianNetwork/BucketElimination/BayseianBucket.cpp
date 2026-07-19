// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include <set>
#include <assert.h>

#include "BayesianNetwork/Internal/BucketElimination/BayesianBucket.h"
#include "BayesianNetwork/Internal/BBN_Node.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		BayesianBucket::BayesianBucket(InferenceNode& aReference)
			: mReference(aReference)
			, mFactor()
			, mChildren()
			, mParents()
			, mSignificantParent(this)
			, mIndexRemapping()
			, mInheritedFactors()

		{
		}

		BayesianBucket::BayesianBucket(BayesianBucket& aRHS)
			: mReference(aRHS.mReference)
			, mFactor(aRHS.mFactor)
			, mChildren(aRHS.mChildren)
			, mParents(aRHS.mParents)
			, mSignificantParent(this)
			, mIndexRemapping(aRHS.mIndexRemapping)
			, mInheritedFactors(aRHS.mInheritedFactors)
		{}

		// good to have, but it exposes a problem with our management of memory stil
		// particularly the factor getting a reference to an internal memory buffer
		// the running results is still in the wrong place!
		BayesianBucket::BayesianBucket(BayesianBucket&& aRHS) noexcept
			: mReference(aRHS.mReference)
			, mFactor(std::move(aRHS.mFactor))
			, mChildren(std::move(aRHS.mChildren))
			, mParents(std::move(aRHS.mParents))
			, mSignificantParent(this)
			, mIndexRemapping(std::move(aRHS.mIndexRemapping))
			, mInheritedFactors(std::move(aRHS.mInheritedFactors))
			{}

		// inefficient, avoid using. Implemented only for DLL Linkage
		BayesianBucket& BayesianBucket::operator=(BayesianBucket& aRHS)
		{
			// we don't want to allow this, but for DLL linkage it throws a compile error not to have it
			assert(false);
			// use placement new to invoke the copy constructore
			return *(new(this)BayesianBucket(aRHS));
		}

		BayesianBucket& BayesianBucket::operator=(BayesianBucket&& aRHS) noexcept
		{
			return *(new(this)BayesianBucket(std::move(aRHS)));
		}

		bool BayesianBucket::HasParent(const BayesianBucket& aTestParent) const
		{
			return mParents.find(&aTestParent) != mParents.end();
		}

		bool BayesianBucket::IsActive() const
		{
			return mMarked;
		}

		bool BayesianBucket::IsObserved() const
		{
			return mReference.IsObserved();
		}

		bool BayesianBucket::AddUniqueParent(BayesianBucket& aNewParent, unsigned int aMask)
		{
			auto [iter, result] = mParents.insert(ParentBuckets::value_type(&aNewParent, {&aNewParent , aMask}));
			return result;
		}

		void BayesianBucket::AddChild(BayesianBucket& aNewChild)
		{
			mChildren.push_back(&aNewChild);
		}

		// TODO: 
		// the management of the running results is not good. Control and ownership is shared between the bucket and the factor
		float BayesianBucket::GetPositiveBelief() const
		{
			return mFactor.GetPositiveBelief();
		}

		float BayesianBucket::GetTotalBelief() const 
		{
			return mFactor.GetTotalBelief();
		}


		// between inferences, we can assume that the graph structure (conditioning cases and nodes/keys) haven't changed,
		// but the observed probabilities and the transient data in the CPTs and buckets should be cleared. This allows us 
		// to reuse the same graph structure for multiple inferences, which is a common use case for this kind of thing
		// NOTE: we specifically do not touch the mark value since that is a specific optimization to clear during the processig 
		// pass so we can accumulate it during the pre-solve preparations
		void BayesianBucket::Reset()
		{
			mFactor.Reset();
			mIndexRemapping.Clear();
			mInheritedFactors.clear();
		}

		void BayesianBucket::Clear()
		{
			Reset();
			mChildren.clear();
			mParents.clear();
			mSignificantParent = this;
			mMarked = false;
		}


		void BayesianBucket::ComputeIndexMappings(KeyList& aAccumulatedIndexes)
		{
			std::set<BBN_Node::NodeKeyType> uniqueKeys;
			// this part sucks since it's always the same, the keys of this node's conditioning table
			// not generally expected to change frequently. 
			for (auto currentKey : mReference.GetConditioningKeys())
			{
				uniqueKeys.insert(currentKey);
			}
			// and gather up the factors
			for (auto currentFactor : mInheritedFactors)
			{
				for (auto factorKey : currentFactor->GetKeys())
				{
					uniqueKeys.insert(factorKey);
				}
			}
			// we want the owner node to have it's key last, so remove it from here if it was added
			// then gather it in an ordered list so that the owner can come last
			// this is pretty redundant since we throw this vector away (and only use it to generate
			// an ordering). A change to the remap table API would let us add the the from entries as 
			// we go, skipping this aAccumulatedIndexes vector completely
			uniqueKeys.erase(mReference.GetKey());
			for (auto key : uniqueKeys)
			{
				aAccumulatedIndexes.push_back(key);
			}
			aAccumulatedIndexes.push_back(mReference.GetKey());
			mIndexRemapping.SetFrom(aAccumulatedIndexes);

			// now create the remaps for each of the associated factors
			for (auto currentFactor : mInheritedFactors)
			{
				mIndexRemapping.ComputeMapping(currentFactor->GetKeys());
			}

			// ending in us again, which again is constant generally
			mIndexRemapping.ComputeMapping(mReference.GetConditioningKeys());
		}

		void BayesianBucket::ComputePermutation()
		{
			mFactor.Reset();
			auto& accumulatedKeys = mFactor.EditKeys();

			ComputeIndexMappings(accumulatedKeys);
			unsigned int tableSize = 1 << accumulatedKeys.size();
			mFactor.Resize(tableSize);

			unsigned int current = 0;
			// there are 2 stages that share most of the code, but it's ultimately not much code and would be 
			// messier to generalize. No real performance impact since the stages are non-overlapping and amount to 
			// once through the table
			// 
			// stage 1
			// compute the result where the node is not true
			unsigned int half = tableSize >> 1;
			for (; current < half; ++current)
			{
				IndexType currentIdx(current);
				IndexType localIndex = RemapToLocalIndex(currentIdx);
				float currentValue = (1.0f - mReference.ComputeRow(localIndex)) * AccumulateFactors(currentIdx);
				mFactor.SetRunningValue(currentIdx, currentValue);
			}

			// stage 2
			// the same but for when the node is true
			for (; current < tableSize; ++current)
			{
				IndexType currentIdx(current);
				IndexType localIndex = RemapToLocalIndex(currentIdx);
				float currentValue = mReference.ComputeRow(localIndex) * AccumulateFactors(currentIdx);
				mFactor.SetRunningValue(currentIdx, currentValue);

			}
			// TODO:
			// still a bit of a hack that I grab this from the factor and populate/manipulate it here. 
			accumulatedKeys.pop_back(); // the last key is the one for the current node, we don't want to factor on that
		}


		BayesianBucket::IndexType BayesianBucket::RemapIndexes(const IndexType aFrom, unsigned int aRemapIdx) const
		{
			IndexType remapped(0);
			unsigned int bitIdx = 0;
			for (auto fromIdx : mIndexRemapping[aRemapIdx])
			{
				remapped.set(bitIdx++, aFrom[fromIdx]);
			}
			return remapped;
		}

		BayesianBucket::IndexType BayesianBucket::RemapToLocalIndex(const IndexType aFrom) const
		{
			// if unspecified then we're interested in the current factor, that is the last added factor's remapping
			unsigned int latestFactor = static_cast<RemapTable::IndexType>(mInheritedFactors.size());
			return RemapIndexes(aFrom, latestFactor);
		}


		float BayesianBucket::AccumulateFactors(const IndexType aIndex) const
		{
			float accumulated = 1.0f;
			unsigned int factorID = 0;
			for (auto* factor : mInheritedFactors)
			{
				IndexType factor_idx = RemapIndexes(aIndex, factorID);

				accumulated *= (*factor)[factor_idx];
				++factorID;
			}
			return accumulated;
		}


	}
}