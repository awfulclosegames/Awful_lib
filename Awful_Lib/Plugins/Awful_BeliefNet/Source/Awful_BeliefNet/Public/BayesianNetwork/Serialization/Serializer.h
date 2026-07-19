#pragma once
#include "BBN_API.h"

#include "AI_Common/Platform/Serialization/BaseStreamWrapper.h"
#include "BayesianNetwork/Internal/CommonTypes.h"
#include "BayesianNetwork/Internal/Utilities/BBN_Mutator.h"


namespace Awful_BeliefNet
{
	namespace Serialization
	{
		class AWFUL_BBN_API Serializer : private BBN_Mutator
		{
			using SUPER = BBN_Mutator;

		public:

			Serializer(BayesianBeliefNetwork& aBBN, ProbabilityCollection* aPriorSets = nullptr, TrainingSet* aTrainingSets = nullptr, MetadataCollection* aMetadata = nullptr)
				: SUPER(aBBN)
				, mPriorSets(aPriorSets)
				, mTrainingSets(aTrainingSets)
				, mMetadata(aMetadata)
			{}
			
			void operator()(Awful::BaseStreamWrapper& aStream)
			{
				if (aStream.IsSaving())
				{
					Serialize(aStream);
				}
				else if (aStream.IsLoading())
				{
					Deserialize(aStream);
				}
			}

		private:
			static uint32_t sSerializerVersion;
			static char sSerializerMagic[8];

			// Technically, the serialize method should be const, but the stream wrapper (for symmetry with in/out) defines
			// a non-const API only. This would be good to revisit later.
			void Serialize(Awful::BaseStreamWrapper& aStream);
			void Deserialize(Awful::BaseStreamWrapper& aStream);

			void DeserializeV1(Awful::BaseStreamWrapper& aStream);
			void DeserializeV1PriorSets(Awful::BaseStreamWrapper& aStream);
			void DeserializeV1TrainingSets(Awful::BaseStreamWrapper& aStream);

			void DeserializeMetaDataSet(Awful::BaseStreamWrapper& aStream);
			void DeserializeV1MetaDataSet(Awful::BaseStreamWrapper& aStream);

			void SerializePriorSets(Awful::BaseStreamWrapper& aStream);
			void SerializeTrainingSets(Awful::BaseStreamWrapper& aStream);
			void SerializeMetaDataSet(Awful::BaseStreamWrapper& aStream);

			void SerializeString(Awful::BaseStreamWrapper& aStream, std::string& aString);
			void DeserializeString(Awful::BaseStreamWrapper& aStream, std::string& aString);

			ProbabilityCollection* mPriorSets = nullptr;
			TrainingSet* mTrainingSets = nullptr;
			MetadataCollection* mMetadata;
		};
	}

}
