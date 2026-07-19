#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>

#include "AI_Common/Platform/Serialization/BaseStreamWrapper.h"



namespace Awful
{
	class STL_InFileStream : public BaseStreamWrapper
	{
	public:
		STL_InFileStream(const std::string& aPath, std::ios::openmode aMode = std::ios::binary)
			: mFileStream(aPath.c_str(), aMode)
		{
			assert(mFileStream.is_open());
		}
		~STL_InFileStream()
		{
			mFileStream.close();
		}
		virtual bool IsSaving() const override { return false; }
		virtual bool IsLoading() const override { return true; }
		virtual BaseStreamWrapper& Serialize(void* aData, size_t Length) override;
	protected:
		virtual BaseStreamWrapper& StreamBool(bool& aData) override;
		virtual BaseStreamWrapper& StreamFloat(float& aData)override;
		virtual BaseStreamWrapper& StreamChar(char& aData) override;
		virtual BaseStreamWrapper& StreamInt(uint32_t& aData) override;
		virtual BaseStreamWrapper& StreamPoolString(PoolString& aData)override;
	private:
		std::ifstream mFileStream;
	};

}