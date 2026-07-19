#pragma once
#include <cstdint>
// just the API, implement derived classes for std:: file streams and Unreal archive streams
#include "AI_Common/Platform/Poolstring.h"

namespace Awful
{

	class BaseStreamWrapper
	{
	public:
		virtual ~BaseStreamWrapper() {}

		virtual bool IsSaving() const = 0;
		virtual bool IsLoading() const = 0;
		friend BaseStreamWrapper& operator<<(BaseStreamWrapper& aStream, bool& aData) { aStream.StreamBool(aData); return aStream; }
		friend BaseStreamWrapper& operator<<(BaseStreamWrapper& aStream, float& aData) { aStream.StreamFloat(aData); return aStream; }
		friend BaseStreamWrapper& operator<<(BaseStreamWrapper& aStream, char& aData) { aStream.StreamChar(aData); return aStream; }
		friend BaseStreamWrapper& operator<<(BaseStreamWrapper& aStream, uint32_t& aData) { aStream.StreamInt(aData); return aStream; }
		friend BaseStreamWrapper& operator<<(BaseStreamWrapper& aStream, PoolString& aData) { aStream.StreamPoolString(aData); return aStream; }
		virtual BaseStreamWrapper& Serialize(void* aData, size_t Length) = 0;
	protected:
		virtual BaseStreamWrapper& StreamBool(bool& aData) = 0;
		virtual BaseStreamWrapper& StreamFloat(float& aData) = 0;
		virtual BaseStreamWrapper& StreamChar(char& aData) = 0;
		virtual BaseStreamWrapper& StreamInt(uint32_t& aData) = 0;
		virtual BaseStreamWrapper& StreamPoolString(PoolString& aData) = 0;
	};


}