// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once
#include <vector>


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
	VectorHeap() = default;
	VectorHeap(Comaprison& aComp) :m_Data(), m_Comaprison(aComp) {}
	VectorHeap(VectorHeap<T, Comaprison>&& aRHS) :m_data(std::move(aRHS.m_data)), m_Comaprison(std::move(aRHS.m_Comaprison)) {}
	VectorHeap(const VectorHeap<T, Comaprison>& aRHS) :m_data(aRHS.m_data), m_Comaprison(aRHS.m_Comaprison) {}

	~VectorHeap() = default;


	void Push(const T& item)
	{
		m_data.push_back(item);
		std::push_heap(m_data.begin(), m_data.end(), m_Comaprison);
	}

	void Pop()
	{
		const size_t curr = m_data.size();
		std::pop_heap(m_data.begin(), m_data.end(), m_Comaprison);
		m_data.pop_back();
	}

	void Push_Unsorted(const T& item)
	{
		m_data.push_back(item);
	}
	void Pop_Unsorted()
	{
		m_data.pop_back();
	}

	void MakeHeap()
	{
		std::make_heap(m_data.begin(), m_data.end(), m_Comaprison);
	}

	void clear()
	{
		m_Data.clear();
	}

	const bool empty() const { return m_data.empty(); }
	const T& top() { return m_data.front(); }

	VectorHeap& operator= (VectorHeap&&) = default;



private:
	VectorHeap(const TypeVector& data);

	TypeVector m_Data;

	Comaprison m_Comaprison;

};




