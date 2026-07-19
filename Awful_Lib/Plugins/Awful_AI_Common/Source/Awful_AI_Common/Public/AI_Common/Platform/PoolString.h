// Copyright Strati D. Zerbinis 2026
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <iostream>

#include "AICommon_API.h"

// Suppress dll linkage warning on internal std containers
#pragma warning(disable: 4251)

// hack simple platform agnostic implementation of a std::string based
// object that maps a string to an int for fast comparison and safe/performant
// by value use. 
// 
// !!!Super bad performance for adds!!!
// 
// Used directly in the base implementation of the observable keys, but that 
// should probably be abstracted out to a more general purpose string pool implementation at some point.

// Unreal implementation will use FName anyway

namespace Awful
{
	class AWFUL_AI_COMMON_API PoolString
	{
	public:
		static PoolString Invalid;

		PoolString()
			: mID(-1)
			, mHash(static_cast<std::size_t>(-1))
#ifdef _DEBUG
			, DEBUG_STRING("INVALID")
#endif
		{
		}

		PoolString(const PoolString& aOther)
			: mID(aOther.mID)
			, mHash(aOther.mHash)
#ifdef _DEBUG
			, DEBUG_STRING(aOther.DEBUG_STRING)
#endif
		{
		}

		PoolString(const std::string& aString)
		{
			*this = aString;
		}

		bool operator==(const PoolString& aRHS) const
		{
			return mID == aRHS.mID;
		}

		bool operator!=(const PoolString& aRHS) const
		{
			return !(*this == aRHS);
		}
		PoolString& operator=(char aRHS)
		{
			std::string temp;
			temp = aRHS;
			*this = temp;
			return *this;
		}

		PoolString& operator=(const std::string& aRHS)
		{
#ifdef _DEBUG
			DEBUG_STRING = aRHS;
#endif
			// if the string already exists just take the index
			int index = 0;
			for (auto& current : sPool)
			{
				if (current == aRHS)
				{
					mID = index;
					mHash = std::hash<int>{}(mID);

					return *this;
				}
				++index;
			}

			// otherwise pool the string and note the index
			mID = static_cast<int>(sPool.size());
			sPool.push_back(aRHS);
			mHash = std::hash<int>{}(mID);

			return *this;
		}

		PoolString& operator=(const PoolString& aRHS)
		{
			mID = aRHS.mID;
			mHash = aRHS.mHash;
#ifdef _DEBUG
			DEBUG_STRING = aRHS.DEBUG_STRING;
#endif
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& stream, const PoolString& ps)
		{
			stream << ps.ToString() << '\0';
			return stream;
		}

		friend std::istream& operator>>(std::istream& stream, PoolString& ps)
		{
			std::string deserializedString;
			std::getline(stream, deserializedString);
			new(&ps)PoolString(deserializedString);
			return stream;
		}

		// file streams need to be a bit more careful
		friend std::ofstream& operator<<(std::ofstream& stream, const PoolString& ps)
		{
			// feels like a real hack that I have to manually manage this 
			// when passing an STL string to an STL stream. This all seems like stuff the 
			// STL implementations should already be doing. 
			const std::string& serializedString = ps.ToString();
			uint32_t length = static_cast<uint32_t>(serializedString.length());
			stream.write(reinterpret_cast<const char*>(&length), sizeof(uint32_t));
			stream.write(serializedString.c_str(), length);
			return stream;

		}

		friend std::ifstream& operator>>(std::ifstream& stream, PoolString& ps)
		{
			std::string deserializedString;
			uint32_t length = 0;
			stream.read(reinterpret_cast<char*>(&length), sizeof(uint32_t));
			if (length > 0)
			{
				deserializedString.resize(length);
				stream.read(deserializedString.data(), length);
			}

			new(&ps)PoolString(deserializedString);
			return stream;
		}


		operator const std::string& () { return ToString(); }
		const std::string& ToString() const { return sPool[mID]; }

		int GetID() const { return mID; }
		std::size_t GetHash() const { return mHash; }
	private:

		static std::vector<std::string> sPool;
		int mID = -1;
		std::size_t mHash = static_cast<std::size_t>(-1);
#ifdef _DEBUG
		std::string DEBUG_STRING;
#endif
	};
}

	template<>
	struct std::hash<Awful::PoolString>
	{
		std::size_t operator()(Awful::PoolString const& p) const noexcept
		{
			return p.GetHash();
		}
	};

	template<>
	struct std::less<Awful::PoolString>
	{
		std::size_t operator()(Awful::PoolString const& A, Awful::PoolString const& B) const noexcept
		{
			return A.GetID() < B.GetID();
		}
	};

	template<>
	struct std::equal_to<Awful::PoolString>
	{
		bool operator()(Awful::PoolString const& A, Awful::PoolString const& B) const
		{
			return A == B;
		}
	};
