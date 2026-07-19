// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once
#include <vector>
#include <algorithm>
#include <stdexcept>

#define AwfulNoThrow

template<typename T>
struct LessThanComp
{
	bool operator()(const T& aLHS, const T& aRHS)
	{
		return aLHS < aRHS;
	}
};


template <typename T, typename Comaprison = LessThanComp<T>>
class VectorHeap
{
private:
	using TypeVector = std::vector<T>;

public:
	using ComparisonType = Comaprison;

	VectorHeap() = default;
	VectorHeap(Comaprison aComp) : m_Data(), m_Comparison(std::move(aComp)) {}
	VectorHeap(VectorHeap<T, Comaprison>&& aRHS) noexcept
		: m_Data(std::move(aRHS.m_Data)), m_Comparison(std::move(aRHS.m_Comparison))
	{}
	VectorHeap(const VectorHeap<T, Comaprison>& aRHS)
		: m_Data(aRHS.m_Data), m_Comparison(aRHS.m_Comparison)
	{}

	~VectorHeap() = default;

	// Reserve capacity to prevent reallocations during batch operations
	void Reserve(size_t aCapacity)
	{
		m_Data.reserve(aCapacity);
	}

	// Shrink to fit to free unused capacity after Clear()
	void ShrinkToFit()
	{
		m_Data.shrink_to_fit();
	}

	// Reset the comparison function (takes by value for move semantics)
	void ResetComparator(Comaprison aComp)
	{
		m_Comparison = std::move(aComp);
	}

	// Individual push (maintains heap property)
	void Push(const T& item)
	{
		m_Data.push_back(item);
		std::push_heap(m_Data.begin(), m_Data.end(), m_Comparison);
	}

	// Individual pop (removes and returns top element)
	T Pop()
	{
#ifndef AwfulNoThrow
		// no throw for games
		if (Empty())
		{
			throw std::out_of_range("VectorHeap::Pop() called on empty heap");
		}
#endif
		std::pop_heap(m_Data.begin(), m_Data.end(), m_Comparison);
		T item = std::move(m_Data.back());
		m_Data.pop_back();
		return item;
	}

	// Push unsorted - for batch operations where you don't need heap property yet
	void Push_Unsorted(const T& item)
	{
		m_Data.push_back(item);
	}

	// Pop unsorted - for batch operations where you don't need heap property
	void Pop_Unsorted()
	{
#ifndef AwfulNoThrow
		// no throw for games
		if (m_Data.empty())
		{
			throw std::out_of_range("VectorHeap::Pop_Unsorted() called on empty heap");
		}
#endif
		m_Data.pop_back();
	}

	// Convert to heap in O(n) time - for batch operations
	void MakeHeap()
	{
		if (!m_Data.empty())
		{
			std::make_heap(m_Data.begin(), m_Data.end(), m_Comparison);
		}
	}

	// Batch push - adds multiple items and makes heap in one operation
	template<typename InputIt>
	void PushBatch(InputIt first, InputIt last)
	{
		if (first == last)
		{
			return;  // No items to add
		}

		// Reserve space if needed
		size_t additionalSize = std::distance(first, last);
		if (m_Data.size() + additionalSize > m_Data.capacity())
		{
			m_Data.reserve(m_Data.capacity() + additionalSize);
		}

		// Append items without heap property
		m_Data.insert(m_Data.end(), first, last);

		// Build heap in O(n) time
		std::make_heap(m_Data.begin(), m_Data.end(), m_Comparison);
	}

	// Batch pop - removes top element multiple times
	void PopBatch(size_t aCount)
	{

#ifndef AwfulNoThrow
		// no throw for games
		if (aCount > Size())
		{
			throw std::out_of_range("VectorHeap::PopBatch() count exceeds heap size");
		}
#endif

		for (size_t i = 0; i < aCount; ++i)
		{
			std::pop_heap(m_Data.begin(), m_Data.end(), m_Comparison);
			m_Data.pop_back();
		}
	}

	// Clear heap and optionally shrink capacity
	void Clear(bool aShrinkToFit = true)
	{
		m_Data.clear();
		if (aShrinkToFit)
		{
			ShrinkToFit();
		}
	}

	// Check if heap is empty
	bool Empty() const noexcept
	{
		return m_Data.empty();
	}

	// Get top element (peek) with bounds checking
	const T& Top() const
	{
#ifndef AwfulNoThrow
		// no throw for games
		if (Empty())
		{
			throw std::out_of_range("VectorHeap::Top() called on empty heap");
		}
#endif
		return m_Data.front();
	}

	// Get reference to top element (mutable)
	T& Top()
	{
#ifndef AwfulNoThrow
		// no throw for games
		if (Empty())
		{
			throw std::out_of_range("VectorHeap::Top() called on empty heap");
		}
#endif
		return m_Data.front();
	}

	// Get current heap size
	size_t Size() const noexcept
	{
		return m_Data.size();
	}

	// Check if heap is valid (heap property maintained)
	bool IsValid() const
	{
		return std::is_heap(m_Data.begin(), m_Data.end(), m_Comparison);
	}

	// Get raw vector (for advanced usage - BE CAREFUL)
	const TypeVector& GetRaw() const noexcept
	{
		return m_Data;
	}

	// Non-const operator[] for direct access (BE CAREFUL - may break invariants)
	TypeVector& operator[](size_t aIndex)
	{
		return m_Data[aIndex];
	}

	// Const operator[] for safe read access
	const T& operator[](size_t aIndex) const
	{
		return m_Data[aIndex];
	}

	VectorHeap& operator=(VectorHeap&&) noexcept = default;
	VectorHeap& operator=(const VectorHeap&) = default;


private:
	VectorHeap(const TypeVector& data);  // Deleted - copy constructor handles it


private:
	TypeVector m_Data;

	Comaprison m_Comparison;

};
