// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

#include <map>

namespace Awful
{
	// A simple tool for managing two way associations between two sets of keys. Not currently designed to be especially performant
	// since it's assumed this will be used outside of hot loops. Also there is an assumption that both keys and values are unique,
	// and appropriate for the container type used. 
	// 
	// Generally useful especially when associating keys from one domain with keys in another.
	// Such as associating the identifiers of nodes in a BBN with the actions that they represent in a decision network.

	template<typename Key_Type, typename Value_Type, template<typename, typename> class MappingContainer = std::map>
	class BidirectionalMap
	{
	public:
		void AddAssociation(const Key_Type& aKey, const Value_Type& aValue)
		{
			mForwardMap.emplace(aKey, aValue);
			mReverseMap.emplace(aValue, aKey);
		}
		const Value_Type* GetValue(const Key_Type& aKey) const
		{
			auto iter = mForwardMap.find(aKey);
			if (iter != mForwardMap.end())
			{
				return &iter->second;
			}
			return nullptr;
		}
		const Key_Type* GetKey(const Value_Type& aValue) const
		{
			auto iter = mReverseMap.find(aValue);
			if (iter != mReverseMap.end())
			{
				return &iter->second;
			}
			return nullptr;
		}

		// This is a common case, so a bit of syntactic sugar for it. 
		// It lacks validation and error handling, also it only works forward. 
		// A reverse version would be easy enough to add, but if the types were the 
		// same it would be ambiguous so I haven't done that yet.
		Value_Type& operator[](const Key_Type& aKey) { return mForwardMap[aKey]; }

		// it's assumed that the maps are symmetric so we can iterate on just one
		using iterator = MappingContainer<Key_Type, Value_Type>::iterator;
		using const_iterator = MappingContainer<Key_Type, Value_Type>::const_iterator;
		iterator begin() { return mForwardMap.begin(); }
		iterator end() { return mForwardMap.end(); }
		const_iterator begin() const { return mForwardMap.begin(); }
		const_iterator end() const { return mForwardMap.end(); }
		
	private:
		MappingContainer<Key_Type, Value_Type> mForwardMap;
		MappingContainer<Value_Type, Key_Type> mReverseMap;
	};
}