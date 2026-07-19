#include "AI_Common/Platform/Serialization/STL_OutFileStream.h"

namespace Awful
{

	BaseStreamWrapper& STL_OutFileStream::Serialize(void* aData, size_t Length)
	{
		mFileStream.write(reinterpret_cast<char*>(aData), Length);
		return *this;
	}

	BaseStreamWrapper& STL_OutFileStream::StreamBool(bool& aData)
	{
		return Serialize(&aData, sizeof(bool));
	}

	BaseStreamWrapper& STL_OutFileStream::StreamFloat(float& aData)
	{
		return Serialize(&aData, sizeof(float));

	}

	BaseStreamWrapper& STL_OutFileStream::StreamChar(char& aData)
	{
		return Serialize(&aData, sizeof(char));

	}

	BaseStreamWrapper& STL_OutFileStream::StreamInt(uint32_t& aData)
	{
		return Serialize(&aData, sizeof(uint32_t));

	}

	BaseStreamWrapper& STL_OutFileStream::StreamPoolString(PoolString& aData)
	{
		mFileStream << aData;
		return *this;
	}
}