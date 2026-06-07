// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

// evaluate against more data. Initial testing shows that
// standard map outperforms unordered map here for this codebase
#include <map>
#include "CommonDataTypes/Observable.h"


namespace Awful
{
	template<typename Key_Type = PoolString, typename Value_Type = bool>
	class ConditionBase
	{
	public:
		using ObservableType = ObservableBase<Key_Type, Value_Type>;
		// consider making the map to the observable itself rather than just it's data
		// currently we assume that the observables are only key-value pairs with no other
		// functionality or data (even though they do cache a key hash value) 
		// it might be desirable to return the actual observables not just an associated value
		using ConditionType = std::map<typename ObservableType::KeyType, typename ObservableType::ValueType>;
		class ResultToken : private ConditionType::const_iterator
		{
		private:
			friend class ConditionBase;
			ResultToken(ConditionType::const_iterator aRHS)
				: ConditionType::const_iterator(aRHS)
			{
			}
		};

		ConditionType::const_iterator begin() const { return mConditions.begin(); }
		ConditionType::const_iterator end() const { return mConditions.end(); }
		std::size_t size() const { return mConditions.size(); }

		bool empty() const { return mConditions.empty(); }

		// separating out the FIND call makes the usage a bit clumsier but it 
		// prevents the need to hash/search twice for a contains/matches test, and
		// limits the possibilities to abuse the find result
		ResultToken Find(const ObservableType& aObs) const { return mConditions.find(aObs.GetKey()); }
		ResultToken Find(const ObservableType::KeyType aKey) const { return mConditions.find(aKey); }

		ObservableType::ValueType GetValue(ResultToken aSearchResult) const { return aSearchResult->second; }

		// returns true if the condition contains the test value and the values
		// match
		bool Matches(const ObservableType& aObs, ResultToken aSearchResults) const
		{
			return (aSearchResults != mConditions.end()) && (aSearchResults->second == aObs.GetValue());
		}

		// find and matches in one call, convenience function
		bool Contains(const ObservableType& aObs) const
		{
			auto found = mConditions.find(aObs.GetKey());
			return (found != mConditions.end()) && (found->second == aObs.GetValue());
		}

		bool ContainsKey(const ObservableType& aObs) const
		{
			return mConditions.contains(aObs.GetKey());
		}

		// returns true if the condition contains the test key regardless of 
		// the respective values
		bool Valid(const ResultToken aSearchResults) const
		{
			return aSearchResults != mConditions.end();
		}

		void Add(ObservableType& aObs)
		{
			mConditions.insert_or_assign(aObs.GetKey(), aObs.GetValue());
		}

		void Add(const ObservableType& aObs)
		{
			mConditions.insert_or_assign(aObs.GetKey(), aObs.GetValue());
		}

		void Clear() { mConditions.clear(); }
	private:
		ConditionType mConditions;
	};

	class Condition : public ConditionBase<>
	{};
}