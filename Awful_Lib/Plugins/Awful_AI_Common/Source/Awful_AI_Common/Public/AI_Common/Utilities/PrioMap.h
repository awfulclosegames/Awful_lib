// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "AI_Common/Utilities/VectorHeap.h"

#include <unordered_map>
#include <cassert>


template<typename T>
struct MoreThanComp
{
	bool operator()(const T& aLHS, const T& aRHS)
	{
		return aLHS > aRHS;
	}
};

template<typename KT, typename VT, typename ValueComparison = MoreThanComp<typename VT>>
class PrioMap
{
private:
	using QueuePair = std::pair<KT, VT>;

	struct QueuePairCompare
	{
		bool operator ()(const QueuePair& lhs, const QueuePair& rhs) { return (*Comp)(lhs.second, rhs.second); }
		bool operator ()(const QueuePair* lhs, const QueuePair* rhs) { return (*Comp)(lhs->second, rhs->second); }
		ValueComparison* Comp = nullptr;
	} m_StandardComparator;

public:
	using KeyType = KT;
	using ValueType = VT;
	using ComparatorType = ValueComparison;

	PrioMap() = default;
	PrioMap(ComparatorType& aComparator)
		: m_EntryMap()
		, m_Queue(m_StandardComparator)
	{
		m_StandardComparator.Comp = &aComparator;
	}
	PrioMap(const PrioMap<KeyType, ValueType, ValueComparison>& aRHS) = default;
	PrioMap(PrioMap<KeyType, ValueType>&& aRHS)
		: m_EntryMap(std::move(aRHS.m_EntryMap))
		, m_Queue(std::move(aRHS.m_Queue))
	{}


	~PrioMap() = default;

	PrioMap& operator= (PrioMap&& aRHS) = default;

	void ResetComparator(ComparatorType& aComp)
	{
		m_StandardComparator.Comp = &aComp;
		m_Queue.ResetComparator(m_StandardComparator);
	}

	void SetInvalidValue(const ValueType& aValue) { m_InvalidValue = aValue; }

	const bool Exists(const KeyType& aKey) const { return m_EntryMap.count(aKey) > 0; }
	const bool IsEmpty() const { return m_EntryMap.empty(); }
	const ValueType& PeekAt(const KeyType& aKey) const { return m_EntryMap.at(aKey).second; }

	const size_t Size() const { return m_EntryMap.size(); }

	void Insert(const KeyType& aKey, const ValueType& aValue)
	{
		QueuePair& newPair = m_EntryMap.emplace(aKey, std::make_pair(aKey, aValue)).first->second;
		m_Queue.Push(newPair);
	}

	void Edit(const KeyType& aKey, const ValueType& aValue)
	{
		m_EntryMap.at(aKey).second = aValue;
		m_Queue.Push(std::make_pair(aKey, aValue));
	}

	void Insert_Unsorted(const KeyType& aKey, const ValueType& aValue)
	{
		QueuePair& newPair = m_EntryMap.emplace(aKey, std::make_pair(aKey, aValue)).first->second;
		m_Queue.Push_Unsorted(newPair);
	}

	void Edit_Unsorted(const KeyType& aKey, const ValueType& aValue)
	{
		m_EntryMap.at(aKey).second = aValue;
		m_Queue.Push_Unsorted(std::make_pair(aKey, aValue));
	}

	const ValueType Pop()
	{
		while (!m_Queue.Empty())
		{
			const QueuePair toPop = m_Queue.Top();

			m_Queue.Pop();
			size_t wasErased = m_EntryMap.erase(toPop.first);
			if (wasErased)
			{
				return toPop.second;
			}
		}

		return m_InvalidValue;
	}


	void PrioSort()
	{
		m_Queue.MakeHeap();
	}

	void Clear()
	{
		m_Queue.Clear();
		m_EntryMap.clear();
	}

private:

	std::unordered_map<KeyType, QueuePair> m_EntryMap;
	VectorHeap<QueuePair, QueuePairCompare> m_Queue;

	ValueType m_InvalidValue = ValueType();
};
