// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>

#include "BayesianNetwork/BucketElimination/InferenceNode.h"
#include "BayesianNetwork/BucketElimination/BayesianBucket.h"
#include "BayesianNetwork/BucketElimination/Utilities/NodeAccessor.h"

namespace Awful_BeliefNet
{
	namespace BucketElimination
	{
		class OrderingList;
		
		class Ordering
		{
		public:
			// this is a hack, these types shouldn't be exposed publicly and the ordering is off of 
			// the inheritance chain. Better would be instead of receiving the node list, that the
			// node list management is done in the inference class that does already know about it
			using NodeList = InferenceNode::NodeListType;
			static constexpr unsigned int InvalidIndex = static_cast<unsigned int>(-1);

			void push_back(BayesianBucket* aBucket) { mOrder.push_back(aBucket); }
			auto& GetBucket(unsigned int aKey) { return mBuckets[aKey]; }

			auto& operator[](std::size_t aKey) { return *(mOrder[aKey]); }
			auto& operator[](std::size_t aKey) const { return *(mOrder[aKey]); }

			auto begin() { return mOrder.begin(); }
			auto begin()const { return mOrder.begin(); }
			auto end() { return mOrder.end(); }
			auto end()const { return mOrder.end(); }
			auto rbegin()const { return mOrder.rbegin(); }
			auto rbegin() { return mOrder.rbegin(); }
			auto rend() { return mOrder.rend(); }
			auto rend()const { return mOrder.rend(); }

			// I really hate converting from size_t to unsigned int, I get why STL went with
			// size_t but it causes a lot of unnecessary warnings and conversions in our code. I only need 32 bits
			unsigned int size() const { return static_cast<unsigned int>(mOrder.size()); }
			void clear();
		private:
			// TODO: Refactor
			friend class OrderingList;
			using BucketStoreType = std::vector<BayesianBucket>;

			void Fill(NodeList& aNodes);

			BucketStoreType mBuckets;
			BayesianBucket::BucketList mOrder;

		};


		class OrderingList
		{
		public:
			using NodeList = Ordering::NodeList;
			using KeyList = InferenceNode::KeyList;

			void Setup(const KeyList& aRoots, NodeList& aNodes);

			auto& operator[](std::size_t aKey) { return mOrderings[aKey]; }

			void Clear() { mOrderings.clear(); }
		private:
			std::vector<Ordering> mOrderings;
		};


	}
}