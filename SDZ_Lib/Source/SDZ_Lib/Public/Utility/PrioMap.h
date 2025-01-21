// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "VectorHeap.h"

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
		operator ()(const QueuePair& lhs, const QueuePair& rhs) { return Comp(lhs.second, rhs.second); }
		operator ()(const QueuePair* lhs, const QueuePair* rhs) { return Comp(lhs->second, rhs->second); }
		ValueComparison Comp;
	};

public:
	using KeyType = KT;
	using ValueType = VT;

	PrioMap() = default;
	PrioMap(const PrioMap<KeyType, ValueType>& aRHS) = default;
	PrioMap(PrioMap<KeyType, ValueType>&& aRHS)
		: m_EntryMap(std::move(aRHS.m_EntryMap))
		, m_Queue(std::move(aRHS.m_Queue))
	{}

	~PrioMap() = default

	PrioMap& operator= (PrioMap&& aRHS) = default;

	void SetInvalidValue(const ValueType& aValue) { m_InvalidValue = aValue; }

	const bool Exists(const KeyType& aKey) const { return m_EntryMap.count(aKey) > 0; }
	const bool IsEmpty() const { return m_EntryMap.empty(); }
	const ValueType& PeekAt(const KeyType& aKey) const { return m_EntryMap.at(aKey).second; }

	const size_t Size() const { return m_EntryMap.size(); }

	void Insert(const KeyType& aKey, const ValueType& aValue)
	{
		QueuePair& newPair = m_EntryMap.emplace(aKey, std::make_pair(aKey, aValue)).first->second;
		m_Queue.push(newPair);
	}

	void Edit(const KeyType& aKey, const ValueType& aValue)
	{
		m_EntryMap.at(aKey).second = aValue;
		m_Queue.push(std::make_pair(aKey, aValue));
	}

	void clear()
	{
		m_Queue.clear();
		m_EntryMap.clear();
	}

private:

	std::unordered_map<KeyType, QueuePair> m_EntryMap;
	HeapVector<QueuePair, QueuePairCompare> m_Queue;

	ValueType m_InvalidValue = ValueType();
};