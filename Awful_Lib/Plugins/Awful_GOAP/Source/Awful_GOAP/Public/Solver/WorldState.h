// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include "CommonDataTypes/Condition.h"
#include "CommonDataTypes/Observable.h"
#include "GraphOptimizer/BasicTypes.h"

// evaluate against more data. Initial testing shows that
// standard map outperforms unordered map here for this codebase
#include <map>
#include <vector>

namespace Awful
{
	class WorldState : public std::map<Observable::KeyType, Observable::ValueType>
	{
	public:
		using BASE = std::map<Observable::KeyType, Observable::ValueType>;
		using iterator = BASE::iterator;
		using const_iterator = BASE::const_iterator;

		WorldState()
			: BASE()
		{
		}

		WorldState(const Condition& aRhs)
			: BASE()
		{
			insert(aRhs);
		}

		void insert(const Condition& aCond)
		{
			// loop through all observables in a condition and add them to the current worldstate
			for (auto& obs : aCond)
				insert(obs);
		}
		void insert(const Observable& aObs) { insert_or_assign(aObs.GetKey(), aObs.GetValue()); }
		void insert(std::pair<const Observable::KeyType, bool>& aObs) { insert_or_assign(aObs.first, aObs.second); }

		iterator find(const Observable& aObs) { return BASE::find(aObs.GetKey()); }
		const_iterator find(const Observable& aObs)const { return BASE::find(aObs.GetKey()); }
		iterator find(const Observable::KeyType& aObsKey) { return BASE::find(aObsKey); }

		void Reset()
		{
			clear();
		}

	};
}