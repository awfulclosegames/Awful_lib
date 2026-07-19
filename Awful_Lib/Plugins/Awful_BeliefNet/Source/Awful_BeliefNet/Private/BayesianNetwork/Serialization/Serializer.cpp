#include "BayesianNetwork/Serialization/Serializer.h"

namespace Awful_BeliefNet
{
	namespace Serialization
	{
		uint32_t Serializer::sSerializerVersion = 1;
		char Serializer::sSerializerMagic[8] = { 'a','w','f','u','l','b','b','n'};

		// Even though the stream interface is symmetric for load/store we implement separate functions for clarity and to allow for versioning
		// We only support serializing the current version but want to deserialize older versions of the data, so we maintain deserialize 
		// functions for each version we support

		void Serializer::Serialize(Awful::BaseStreamWrapper& aStream)
		{
			if (!aStream.IsSaving())
			{
				// silent error, should probably report this somehow
				return;
			}

			// write magic and version
			aStream.Serialize(sSerializerMagic, 8);
			aStream << sSerializerVersion;

			// naive serialization of the BBN, once the identifier is serialized per node all other references can be handled 
			// by compact sequence number (int) and in fact this could be an int 16 or even 8 since we don't expect huge networks.
			// But for now the stream interface only exposes 32 bit ints, and for version 1 we are more concerned with debugging than
			// performance or size. So we will repeatedly use the full IdentifierType (PoolString).

			// start with nodes, and then the conditioning tables, since the CPT reference other nodes
			// for deserializtion it is convenient to have all the nodes loaded before needing to deal with the CPTs
			uint32_t nodeCount = static_cast<uint32_t>(GetNodes().size());
			aStream << nodeCount;
			for (const auto& nodeHandle : GetNodes())
			{
				// serialize the node's ID and prior probability
				auto& node = GetNode(nodeHandle);
				IdentifierType nodeID = node.GetIdentifier();
				float priorProb = node.GetPriorProbability();
				bool isRoot = node.TestFlagValue(BBN_Node::NodeStateFlags::ROOT);
				aStream << nodeID;
				aStream << priorProb;
				aStream << isRoot;
			}

			// Now serialize the conditional probability tables
			// Iterate again through the nodes, identifying the node we're dealing with (though we could just rely on the stability of the
			// node order). This would allow a sparse representation, but we're not going to do that now for simplicity and debugability.
			for (const auto& nodeHandle : GetNodes())
			{
				// serialize the node's ID and prior probability
				auto& node = GetNode(nodeHandle);

				IdentifierType childNodeID = node.GetIdentifier();
				aStream << childNodeID;
				uint32_t conditioningCaseCount = static_cast<uint32_t>(node.GetCPT().Size());
				aStream << conditioningCaseCount;
				for (auto& conditioningCase : node.GetCPT())
				{
					IdentifierType parentID = conditioningCase.node->GetIdentifier();
					float conditionalProb = conditioningCase.probability;
					aStream << parentID;
					aStream << conditionalProb;
				}
			}

			SerializePriorSets(aStream);
			SerializeTrainingSets(aStream);
			SerializeMetaDataSet(aStream);
		}

		void Serializer::SerializePriorSets(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t priorSetCount = 0;
			if (mPriorSets)
			{
				priorSetCount = static_cast<uint32_t>(mPriorSets->size());
				aStream << priorSetCount;
				for (auto& priorSet : *mPriorSets)
				{
					aStream << priorSet.ID;

					uint32_t priorsCount = static_cast<uint32_t>(priorSet.Probabilities.size());
					aStream << priorsCount;
					for (auto& prior : priorSet.Probabilities)
					{
						// This is a bit clumsy since the stream wrapper API does not support const values even for 
						// serialization.
						IdentifierType nodeID = prior.first;
						float probabilityValue = prior.second;
						aStream << nodeID;
						aStream << probabilityValue;
					}
				}
			}
			else
			{
				// no prior sets so record a count of 0
				aStream << priorSetCount;
			}

		}

		void Serializer::SerializeTrainingSets(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t trainingSetCount = 0;
			if (mTrainingSets)
			{
				trainingSetCount = static_cast<uint32_t>(mTrainingSets->size());
				aStream << trainingSetCount;
				for (auto& trainingCase : *mTrainingSets)
				{
					aStream << trainingCase.ID;
					uint32_t observationCount = static_cast<uint32_t>(trainingCase.Observations.size());
					aStream << observationCount;

					for (auto& observation : trainingCase.Observations)
					{
						IdentifierType nodeID = observation.first;
						float probabilityValue = observation.second;
						aStream << nodeID;
						aStream << probabilityValue;
					}
					aStream << trainingCase.ExpectedResult.ID;
					aStream << trainingCase.ExpectedResult.Confidence;
				}
			}
			else
			{
				// no training sets so record a count of 0
				aStream << trainingSetCount;
			}
		}

		void Serializer::SerializeMetaDataSet(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t currentVersion = MetaDataItem::VersionNumber;
			aStream << currentVersion;

			uint32_t metaDataCount = 0;
			if (mMetadata)
			{
				metaDataCount = static_cast<uint32_t>(mMetadata->size());
				aStream << metaDataCount;
				for (auto& metaDataItem : *mMetadata)
				{
					aStream << metaDataItem.Name;
					aStream << metaDataItem.Flags;
					aStream << metaDataItem.XValue;
					aStream << metaDataItem.YValue;

					// special handling for string
					SerializeString(aStream, metaDataItem.Description);
				}
			}
			else
			{
				// no metadata so record a count of 0
				aStream << metaDataCount;
			}
		}

		void Serializer::Deserialize(Awful::BaseStreamWrapper& aStream)
		{
			if (!aStream.IsLoading())
			{
				// silent error, should probably report this somehow
				return;
			}

			// read magic and dispatch to the appropriate version
			uint32_t inVersion = -1;
			char inMagic[8] = { 0 };

			aStream.Serialize(inMagic, 8);
			aStream << inVersion;
			if (memcmp(inMagic, sSerializerMagic, 8) != 0)
			{
				// Wrong file type
				// silent error, should probably report this somehow
				return;
			}

			switch (inVersion)
			{
			case 1:
				DeserializeV1(aStream);
				break;
			default:
				// Unsupported file version
				// silent error, should probably report this somehow
				return;
			}
		}

		void Serializer::DeserializeV1(Awful::BaseStreamWrapper& aStream)
		{
			// implement deserialization for version 1 here
			
			// naive serialization of the BBN, once the identifier is serialized per node all other references can be handled 
			// by compact sequence number (int) and in fact this could be an int 16 or even 8 since we don't expect huge networks.
			// But for now the stream interface only exposes 32 bit ints, and for version 1 we are more concerned with debugging than
			// performance or size. So we will repeatedly use the full IdentifierType (PoolString).

			uint32_t nodeCount = 0;
			aStream << nodeCount;
			for (uint32_t i = 0; i < nodeCount; ++i)
			{
				IdentifierType nodeID;
				float priorProb = 0.0f;
				bool isRoot = false;
				aStream << nodeID;
				aStream << priorProb;
				aStream << isRoot;
				// Create the node in the BBN
				if (isRoot)
				{
					CreateRootNode(nodeID, priorProb);
				}
				else
				{
					CreateEvidenceNode(nodeID, priorProb);
				}
			}

			// Now deserialize the conditional probability tables
			// We know there will be one record for each node, even if it has no conditioning cases
			for (uint32_t i = 0; i < nodeCount; ++i)
			{
				// Technically this is completely redundant, but it's an extra check for debugging and error detection
				IdentifierType childNodeID;
				uint32_t conditioningCaseCount = 0;
				aStream << childNodeID;
				aStream << conditioningCaseCount;
				for (uint32_t j = 0; j < conditioningCaseCount; ++j)
				{
					IdentifierType parentNodeID;
					float conditionalProb = 0.0f;
					aStream << parentNodeID;
					aStream << conditionalProb;
					// Add the conditional probability to the BBN
					AddConditionalProbability(parentNodeID, childNodeID, conditionalProb);
				}
			}

			DeserializeV1PriorSets(aStream);
			DeserializeV1TrainingSets(aStream);
			DeserializeMetaDataSet(aStream);
		}

		void Serializer::DeserializeV1PriorSets(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t priorSetCount = 0;
			aStream << priorSetCount;

			// If we don't have a prior set collection, we can't deserialize it, so just read the count and discard the data
			// We could just seek past this, but for V1 we want to favour simplicity and debugability over performance
			bool hasPriorSets = (mPriorSets != nullptr);
			for (uint32_t i = 0; i < priorSetCount; ++i)
			{
				IdentifierType priorSetID;
				aStream << priorSetID;
				ProbabilitySet probabilities;
				uint32_t priorsCount = 0;
				aStream << priorsCount;
				for (uint32_t j = 0; j < priorsCount; ++j)
				{
					IdentifierType nodeID;
					float probabilityValue = 0.0f;
					aStream << nodeID;
					aStream << probabilityValue;
					probabilities.Add({ nodeID, probabilityValue });
				}
				if (hasPriorSets)
				{
					// Note, more efficient to emplace_back, but we want to be able to debug this and see the copy constructor being called
					mPriorSets->push_back({ priorSetID, probabilities });
				}
			}
		}

		void Serializer::DeserializeV1TrainingSets(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t trainingSetCount = 0;
			aStream << trainingSetCount;

			// If we don't have a prior set collection, we can't deserialize it, so just read the count and discard the data
			// We could just seek past this, but for V1 we want to favour simplicity and debugability over performance
			bool hasTrainingSets = (mTrainingSets != nullptr);
			for (uint32_t i = 0; i < trainingSetCount; ++i)
			{
				IdentifierType trainingCaseID;
				aStream << trainingCaseID;
				ProbabilitySet observations;
				uint32_t trainingObservationsCount = 0;
				aStream << trainingObservationsCount;
				for (uint32_t j = 0; j < trainingObservationsCount; ++j)
				{
					IdentifierType nodeID;
					float probabilityValue = 0.0f;
					aStream << nodeID;
					aStream << probabilityValue;
					observations.Add({ nodeID, probabilityValue });
				}
				TrainingResult expectedResult;
				aStream << expectedResult.ID;
				aStream << expectedResult.Confidence;
				if (hasTrainingSets)
				{
					mTrainingSets->push_back({ trainingCaseID, observations, expectedResult });
				}
			}
		}

		void Serializer::DeserializeMetaDataSet(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t currentVersion = 0;
			aStream << currentVersion;

			switch (currentVersion)
			{
			case 1:
				DeserializeV1MetaDataSet(aStream);
				break;
			default:
				// Unsupported metadata version
				// silent error, should probably report this. 
				// also this is the (currently) last blcok in the file so we can just stop
				// but if we add more data this will need to skip any data in this section
				return;
			}
		}

		void Serializer::DeserializeV1MetaDataSet(Awful::BaseStreamWrapper& aStream)
		{
			uint32_t metaDataCount = 0;
			aStream << metaDataCount;
			bool hasMetadata = (mMetadata != nullptr);
			for (uint32_t i = 0; i < metaDataCount; ++i)
			{
				MetaDataItem metaDataItem;
				aStream << metaDataItem.Name;
				aStream << metaDataItem.Flags;
				aStream << metaDataItem.XValue;
				aStream << metaDataItem.YValue;

				// special handling for string
				DeserializeString(aStream, metaDataItem.Description);
				if (hasMetadata)
				{
					mMetadata->push_back(metaDataItem);
				}
			}
		}

		void Serializer::SerializeString(Awful::BaseStreamWrapper& aStream, std::string& aString)
		{
			// based on the stream handling in the stl implementation of the PoolString. 
			
			// feels like a real hack that I have to manually manage this 
			// when passing an STL string to an STL stream. This all seems like stuff the 
			// STL implementations should already be doing. 
			uint32_t length = static_cast<uint32_t>(aString.length());
			aStream << length;
			aStream.Serialize(aString.data(), length);
		}

		void Serializer::DeserializeString(Awful::BaseStreamWrapper& aStream, std::string& aString)
		{
			// based on the stream handling in the stl implementation of the PoolString. 
			uint32_t length = 0;
			aStream << length;
			if (length > 0)
			{
				aString.resize(length);
				aStream.Serialize(aString.data(), length);
			}
		}

	}
}

