// Copyright Strati D. Zerbinis 2026. All Rights Reserved.

#pragma once

#include "Utilities/Pooling/BasicPool.h"
#include "Solver/WorldStateStack.h"

namespace Awful
{
	// Global pool instance for shared access across all WorldStateStack instances
	class WorldStateStackDataBlockPoolHolder
	{
	public:
		static WorldStateStackDataBlockPool& GetInstance()
		{
			static WorldStateStackDataBlockPool sInstance(1024);
			return sInstance;
		}

		// Reset the global pool (call during shutdown or when rebuilding world)
		static void ResetGlobalPool()
		{
			GetInstance().Clear();
		}

	private:
		WorldStateStackDataBlockPoolHolder() = delete;
		~WorldStateStackDataBlockPoolHolder() = delete;
		WorldStateStackDataBlockPoolHolder(const WorldStateStackDataBlockPoolHolder&) = delete;
		WorldStateStackDataBlockPoolHolder& operator=(const WorldStateStackDataBlockPoolHolder&) = delete;
	};

}