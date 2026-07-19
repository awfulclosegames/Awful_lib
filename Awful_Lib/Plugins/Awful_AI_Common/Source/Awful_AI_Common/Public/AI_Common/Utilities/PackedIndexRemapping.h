// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <map>

namespace Awful
{
	// TODO:
	// this class has significant room for improvement in terms of performance
	// 
	// This class creates a packed mapping of just the matched keys into ordered
	// indexes
	template<typename Key_Type, typename Value_Type = unsigned int>
	class PackedIndexRemapping
	{
	public:
		using KeyType = Key_Type;
		using IndexType = Value_Type;
		using KeyList = std::vector<KeyType>;
		using IndexList = std::vector<IndexType>;

		// shortcut / hack
		// since this list will generally not be changing for an entire set of mappings so
		// let the caller set it up once. 
		void SetFrom(const KeyList& aFrom);

		void Reserve(unsigned int aSize) { mMappings.reserve(aSize); }
		void ComputeMapping(const KeyList& aTo);

		const IndexList& operator[](IndexType aFrom) const { return mMappings[aFrom]; }
		void Clear() { mMappings.clear(); mFromIndices.clear(); }
		unsigned int Size() const { return static_cast<unsigned int>(mMappings.size()); }


	private:
		std::vector<IndexList> mMappings;
		std::map<KeyType, IndexType> mFromIndices;

	};

	template<typename Key_Type, typename Value_Type>
	void PackedIndexRemapping<Key_Type, Value_Type>::SetFrom(const KeyList& aFrom)
	{
		IndexType order = 0;
		for (Key_Type index : aFrom)
		{
			mFromIndices.emplace(index, order++);
		}
	}

	template<typename Key_Type, typename Value_Type>
	void PackedIndexRemapping<Key_Type, Value_Type>::ComputeMapping(const KeyList& aTo)
	{
		mMappings.emplace_back();
		auto& currentMapping = mMappings.back();

		for (Key_Type CurrentTo : aTo)
		{
			auto contains = mFromIndices.find(CurrentTo);
			if (contains != mFromIndices.end())
			{
				currentMapping.push_back(contains->second);
			}
		}
	}

}