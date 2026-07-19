// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "AI_Common/Platform/PoolString.h"
#include "AI_Common/CommonDataTypes/ContinuousCondition.h"

namespace Awful_BeliefNet
{
	// for associating nodes with external data, also a human readable ID for nodes
	using IdentifierType = typename Awful::PoolString;

	using ProbabilitySet = typename Awful::ContinuousCondition;
	 
	using SequenceKey = unsigned int;
	struct ProbabilitySetWithID
	{
		IdentifierType ID;
		ProbabilitySet Probabilities;
	};
	using ProbabilityCollection = std::vector<ProbabilitySetWithID>;

	struct TrainingResult
	{
		IdentifierType ID;
		float Confidence = 0.0f;
	};

	struct TrainingCase
	{
		IdentifierType ID;
		ProbabilitySet Observations;
		TrainingResult ExpectedResult;
	};

	using TrainingSet = std::vector<TrainingCase>;


	struct MetaDataItem
	{
		// The node, prior set, or training case that this metadata is associated with
		IdentifierType Name;

		// any flags or useful information, particularly for the editor
		uint32_t Flags = 0;

		// Useful for recording the editor location of individual nodes, or the min/max weights for a prior set, 
		// or the expected impact for a training case.
		float XValue = 0.0f;
		float YValue = 0.0f;

		// a human readable description of the node, prior set, or training case
		// TODO: this might need a refactor or a more abstracted approach to support Unreal as well
		// as the freestanding editor and other non-Unreal uses.
		std::string Description;

		// allow versioning the metadata independently from the rest of the data in the file, since
		// this could change frequently. Especially at first
		static const uint32_t VersionNumber = 1;
	};


	using MetadataCollection = std::vector<MetaDataItem>;

}
