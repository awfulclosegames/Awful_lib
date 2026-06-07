// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <functional>

#include "Platform/PoolString.h"


namespace Awful
{
	// refactor to allow supplying a generic key type
	template<typename Key_Type = PoolString, typename Value_Type = bool>
	class ObservableBase
	{
	public:
		using KeyType = PoolString;
		using ValueType = Value_Type;

		ObservableBase(const KeyType& aKey, ValueType aVal)
			: mName(aKey)
			, mValue(aVal)
		{
			ComputeHash();
		}

		ObservableBase(const std::pair<const ObservableBase::KeyType, ObservableBase::ValueType>& aObs)
			: mName(aObs.first)
			, mValue(aObs.second)
		{
			ComputeHash();
		}

		ObservableBase(const ObservableBase& aObs)
			: mName(aObs.mName)
			, mValue(aObs.mValue)
			, mHash(aObs.mHash)
		{
		}

		ObservableBase(ObservableBase&& aObs)
			: mName(std::move(aObs.mName))
			, mValue(aObs.mValue)
			, mHash(aObs.mHash)
		{
		}

		const ObservableBase& operator!() const
		{
			return { mName , !mValue, mHash };
		}

		bool operator==(const ObservableBase& aRHS) const
		{
			return mName == aRHS.mName && mValue == aRHS.mValue;
		}

		std::size_t GetHash() const { return mHash; }

		const KeyType& GetKey() const { return mName; }
		const PoolString& GetName() const { return mName; }
		ValueType GetValue() const { return mValue; }

	private:
		ObservableBase(KeyType aKey, ValueType aVal, std::size_t aHash)
			: mName(aKey)
			, mValue(aVal)
			, mHash(aHash)
		{
		}

		PoolString mName;
		ValueType mValue = false;

		void ComputeHash() { mHash = std::hash<KeyType>{}(mName); }
		std::size_t mHash = 0;
	};

	using Observable = ObservableBase<>;
}

template<typename Key_Type, typename Value_Type>
struct std::hash<Awful::ObservableBase<Key_Type, Value_Type>>
{
	std::size_t operator()(const Awful::ObservableBase<Key_Type, Value_Type>& p) const noexcept
	{
		return  p.GetHash();
	}
};

template<typename Key_Type, typename Value_Type>
struct std::less<Awful::ObservableBase<Key_Type, Value_Type>>
{
	std::size_t operator()(Awful::ObservableBase<Key_Type, Value_Type> const& A, Awful::ObservableBase<Key_Type, Value_Type> const& B) const noexcept
	{
		return std::less<Awful::ObservableBase<Key_Type, Value_Type>::KeyType>{}(A.GetKey(), B.GetKey());
	}
};

// Want to make sure that we match only on the key. This way if the test condition
// contains the same key but with a different value it doesn't appears as a missmatch
// not an absence
template<typename Key_Type, typename Value_Type>
struct std::equal_to<Awful::ObservableBase<Key_Type, Value_Type>>
{
	bool operator()(Awful::ObservableBase<Key_Type, Value_Type> const& A, Awful::ObservableBase<Key_Type, Value_Type> const& B) const
	{
		return A == B;
	}
};

