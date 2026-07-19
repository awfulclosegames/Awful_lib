#pragma once

#include <vector>
#include "AI_Common/Utilities/CommonEntry.h"


// using a KD-Tree as a simple way to perform spatial partitioning
// this is a purpose built implementation specific to the CommonEntry defined as
// a base type in 3 dimensions
// 
// At some point I should make this truly generic and then derive more specific version.
// 
// adding a unique index to the entries could let us reduce the memory (since 
// enough platforms are 64 bit). Alternately, with some more iteration time
// this could be rewritten to work entirely off of spatial information 
// (treating any entries that share position/velocity as effectively identical)
//
// note building the balanced tree has a different signature, this is because 
// this method reorders the input list as it's being inserted, in order to 
// find the median entry at each level


// I'm not exclusive to Unreal right now so I still want to support standard library calls.
// If I ever change my mind, I should go through this and rewrite for Unreal containers etc

namespace Awful
{


	class CommonKDTree
	{
	public:
		void MakeTree(const CommonWorkingSet& items);
		void MakeBalancedTree(CommonWorkingSet& items);
		void SearchRadius(const Vec3f& target, CommonWorkingSet& results, float radius) const;
		void SearchKNN(const Vec3f& target, CommonWorkingSet& results, int count, float radius = FLT_MAX) const;

	private:
		// bit limit is int four 00000100 which (when masked to the low two bits which represent our index space)
		// it the last bit in our rotation
		static constexpr short BitLimit = 4;
		static constexpr short DimensionMask = 3;
		// a simple method to rotate through the indices of the 3 dimensions of the position vector
		inline short RotateDimension(short dim) const { return ((dim & DimensionMask) << 1) | ((dim & BitLimit) >> 2); }

		// for the KNN search we want to maintain a prio queue 
		struct PrioEntry
		{
			PrioEntry(CommonEntry* aB, float aD)
				: m_payload(aB)
				, m_dist(aD)
			{
			}
			CommonEntry* m_payload = nullptr;
			float m_dist = FLT_MAX;
		};
		using PrioQueue = std::vector<PrioEntry>;

		struct TreeNode
		{
		public:
			TreeNode(CommonEntry* key)
				: m_key(key)
				, m_pos(key->GetPos())
			{
			}
			CommonEntry* m_key = nullptr;

			Vec3f m_pos;

			uint32_t m_left = -1;
			uint32_t m_right = -1;
		};

		int Insert(CommonWorkingSet::iterator start, CommonWorkingSet::iterator end, short dim = 1);
		void Insert(CommonEntry* target, uint32_t node = 0, short dim = 1);
		void SearchR(const Vec3f& target, CommonWorkingSet& results, float radius, uint32_t node = 0, short dim = 1) const;
		void SearchK(const Vec3f& target, PrioQueue& results, int count, float radius = FLT_MAX, uint32_t node = 0, short dim = 1) const;

		std::vector<TreeNode> m_tree;

	};

}