#include "AI_Common/Platform/Serialization/STL_InFileStream.h"

namespace Awful
{

	BaseStreamWrapper& STL_InFileStream::Serialize(void* aData, size_t Length)
	{
		mFileStream.read(reinterpret_cast<char*>(aData), Length);
		return *this;
	}

	BaseStreamWrapper& STL_InFileStream::StreamBool(bool& aData)
	{
		return Serialize(&aData, sizeof(bool));
	}

	BaseStreamWrapper& STL_InFileStream::StreamFloat(float& aData)
	{
		return Serialize(&aData, sizeof(float));

	}

	BaseStreamWrapper& STL_InFileStream::StreamChar(char& aData)
	{
		return Serialize(&aData, sizeof(char));

	}

	BaseStreamWrapper& STL_InFileStream::StreamInt(uint32_t& aData)
	{
		return Serialize(&aData, sizeof(uint32_t));

	}

	BaseStreamWrapper& STL_InFileStream::StreamPoolString(PoolString& aData)
	{
		mFileStream >> aData;
		return *this;
	}
}