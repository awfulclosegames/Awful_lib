// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "ActionSpaceSearch/ActionMetadata.h"


namespace Awful
{
	namespace ActionSpaceSearch
	{
		// Rotate bytes left by one. Left most byte becomes the right most byte.
		std::size_t RotateHashBytes(std::size_t aHash)
		{
			static constexpr int NumBytesToShift = 1;
			static constexpr int CHashMaskUpShift = NumBytesToShift * 8;
			static constexpr int CHashMaskDownShift = (sizeof(std::size_t) - NumBytesToShift) * 8; // number of bits to shift the top byte to the bottom
			// I don't actually need this mask but, this is the part I'm moving down
			//static constexpr std::size_t CHashHighByteMask = ((std::size_t)(-1) >> CHashMaskDownShift) << CHashMaskDownShift;
			
			return (aHash >> CHashMaskDownShift) | (aHash << CHashMaskUpShift);
		}

		void ActionMetadata::AddToExpectedWorldExplicit(const Condition& aOther)
		{
			auto& expectedWorld = GetStorage().mExpectedWorld;

			// push a new layer so we can add these values ontop 
			expectedWorld = expectedWorld.AcquireLayer();
			expectedWorld.insertExact(aOther);
			mHash = expectedWorld.GetHash();
		}

		void ActionMetadata::AddToExpectedWorld(const Condition& aOther)
		{
			auto& expectedWorld = GetStorage().mExpectedWorld;

			// push a new layer so we can add these values ontop 
			expectedWorld = expectedWorld.AcquireLayer();
			expectedWorld.insert(aOther);
			mHash = expectedWorld.GetHash();
		}

		void ActionMetadata::Instantiate(ActionMetadataPool& aPool)
		{
			mPool = &aPool;
			mStorageID = mPool->Acquire();
		}

		void ActionMetadata::Deinstantiate()
		{
			mPool->Release(mStorageID);
			mPool = nullptr;
			mStorageID = ActionMetadataPool::ItemHandle();
		}

		void ActionMetadata::Reset()
		{
			GetStorage().Clear();
		}

	}
}