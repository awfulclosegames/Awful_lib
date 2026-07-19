
#include <algorithm>
#include "AI_Common/Utilities/CommonKDTree.h"
#include "AI_Common/Utilities/CommonEntry.h"

namespace Awful
{

	void CommonKDTree::MakeTree(const CommonWorkingSet& items)
	{
		m_tree.clear();
		m_tree.reserve(items.size());

		// push the root
		m_tree.emplace_back(items[0]);

		// this could produce imbalanced trees. A more reliable approach is
		// to sort the independent axis before insertion. Though that would 
		// be both a bit more complex and a bit more initial overhead

		for (int i = 1; i < static_cast<int>(items.size()); ++i)
		{
			Insert(items[i]);
		}
	}

	void CommonKDTree::MakeBalancedTree(CommonWorkingSet& items)
	{
		m_tree.clear();
		m_tree.reserve(items.size());


		// this could produce imbalanced trees. A more reliable approach is
		// to sort the independent axis before insertion. Though that would 
		// be both a bit more complex and a bit more initial overhead

		Insert(items.begin(), items.end());
	}


	void CommonKDTree::SearchRadius(const Vec3f& target, CommonWorkingSet& results, float radius) const
	{
		if (m_tree.empty())
			return;

		// from this point on we work in square radius
		SearchR(target, results, radius * radius);
	}


	void CommonKDTree::SearchKNN(const Vec3f& target, CommonWorkingSet& results, int count, float radius) const
	{
		if (m_tree.empty())
			return;

		if (radius < FLT_MAX)
			radius *= radius;

		PrioQueue queue;
		queue.reserve(count);
		SearchK(target, queue, count, radius);

		for (auto& entry : queue)
		{
			results.push_back(entry.m_payload);
		}
	}

	int CommonKDTree::Insert(CommonWorkingSet::iterator start, CommonWorkingSet::iterator end, short dim)
	{
		if (start >= end)
			return -1;
		short dimIndex = dim & DimensionMask;

		auto comp = [dimIndex](const CommonEntry* a, const CommonEntry* b)
			{return a->GetPos()[dimIndex] < b->GetPos()[dimIndex]; };

		auto mid = start + ((end - start) / 2);

		std::nth_element(start, mid, end, comp);

		int returnIndex = static_cast<uint32_t>(m_tree.size());
		m_tree.emplace_back(*mid);
		auto& n = m_tree.back();

		// now add children
		n.m_left = Insert(start, mid, RotateDimension(dim));
		n.m_right = Insert(mid + 1, end, RotateDimension(dim));

		return returnIndex;
	}

	void CommonKDTree::Insert(CommonEntry* target, uint32_t node, short dim)
	{
		// this is structured as a tail recursion 
		TreeNode& n = m_tree[node];
		short dimIndex = dim & DimensionMask;
		if (target->GetPos()[dimIndex] < n.m_key->GetPos()[dimIndex])
		{
			if (n.m_left == -1)
			{
				// ran out of tree, insert on the left
				n.m_left = static_cast<uint32_t>(m_tree.size());
				m_tree.emplace_back(target);
				return;
			}
			// continue to descend the tree, switching testing on the next dimension
			return Insert(target, n.m_left, RotateDimension(dim));
		}

		// not left, so must be right!
		if (n.m_right == -1)
		{
			// ran out of tree, insert on the right
			n.m_right = static_cast<uint32_t>(m_tree.size());
			m_tree.emplace_back(target);
			return;
		}
		// continue to descend the tree, switching testing on the next dimension
		Insert(target, n.m_right, RotateDimension(dim));
	}


	// similar to the insert, but on the way down we collect any nodes that are within radius, and also we descend
	// any branches that define planes which intersect the search sphere
	void CommonKDTree::SearchR(const Vec3f& target, CommonWorkingSet& results, float radius, uint32_t node, short dim) const
	{
		const TreeNode& n = m_tree[node];
		short dimIndex = dim & DimensionMask;
		float dist = (n.m_pos - target).SquaredLength();

		if (dist < radius)
		{
			results.push_back(n.m_key);
		}

		float seperation = target[dimIndex] - n.m_pos[dimIndex];

		bool goLeft = seperation < 0;
		seperation *= seperation;

		auto next = n.m_right;
		auto alternate = n.m_left;
		if (goLeft)
		{
			next = n.m_left;
			alternate = n.m_right;
		}

		// the next link represents the natural path of descent
		// if this node has something on it's next link then descend
		if (next != -1)
		{
			SearchR(target, results, radius, next, RotateDimension(dim));
		}

		// if the seperation plane intersects the search radius then descend the other link too if it exists
		if ((alternate != -1) && (seperation < radius))
		{
			SearchR(target, results, radius, alternate, RotateDimension(dim));
		}
	}



	void CommonKDTree::SearchK(const Vec3f& target, PrioQueue& results, int count, float radius, uint32_t node, short dim) const
	{
		struct Comparator
		{
			bool operator()(const PrioEntry& a, const PrioEntry& b)const
			{
				return a.m_dist < b.m_dist;
			}
		};

		const TreeNode& n = m_tree[node];
		short dimIndex = dim & DimensionMask;

		float seperation = target[dimIndex] - n.m_pos[dimIndex];

		bool goLeft = seperation < 0;

		seperation *= seperation;

		uint32_t next = n.m_right;
		uint32_t alternate = n.m_left;
		if (goLeft)
		{
			next = n.m_left;
			alternate = n.m_right;
		}

		// if there is a branch on this side keep searching
		if (next != -1)
		{
			SearchK(target, results, count, radius, next, RotateDimension(dim));
		}

		// try and add a point on the unwinding of the recursion in order to reduce the
		// search radius as quickly as possible
		float dist = (n.m_pos - target).SquaredLength();
		if (dist < radius)
		{
			results.emplace_back(n.m_key, dist);
			std::push_heap(results.begin(), results.end(), Comparator());
			if (static_cast<int>(results.size()) > count)
			{
				// drop the biggest one if we have our count already
				std::pop_heap(results.begin(), results.end(), Comparator());
				results.pop_back();
				radius = results.front().m_dist;
			}
		}

		// if the separating plane intersects our radius (search bubble) then also look on the 
		// other side
		if ((alternate != -1) && (seperation < radius))
		{
			SearchK(target, results, count, radius, alternate, RotateDimension(dim));
		}
	}

}