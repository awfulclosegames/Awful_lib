// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <unordered_set>
#include <assert.h>

#include "BBN_API.h"

#include "AI_Common/Utilities/Pooling/ChunkPool.h"
#include "AI_Common/CommonDataTypes/Condition.h"
#include "BayesianNetwork/Internal/CommonTypes.h"
#include "BayesianNetwork/Internal/BBN_Node.h"

using namespace Awful;
namespace Awful_BeliefNet
{
	class BBN_Accessor;

	// potentially wasted space, since most BBNs are expected to be small, but this gives us
	// contiguous storage and reference stability, which is important for the CPTs. Also it lets
	// us have larger networks if we like without significant performance degradation.
	// most runtime use of the BBN will come with fixed and known sized sets of nodes which would
	// be fine in a vector. But this makes editing and training easier. Also it gives us the option
	// to do these things at runtime
	// I would like to make this private to the BayesianBeliefNetwork, but the node needs to expose 
	// a default constructor to allow pooling, and I want to limit the ability to construct nodes
	class NodeStoreType : public Awful::ChunkedPool<BBN_Node, 32>
	{ };

	// Base class for a BBN node graph. 
	// This class provides only the structural elements of, and access to, the belief nodes but
	// does not perform inference itself. 
	// The actual work of the inference query is provided by a separate class, as is the file IO 
	// and editor/authoring support
	//
	// At a high level the structure of the graph is as follows:
	// Roots:			- Generally what we are trying to infer about
	//					- Nodes that have no conditional probability tables (no nodes 'above' them)
	//					- Not directly observed
	//					- Representing causes (diagnostic) or action/goals (analytic) that correspond to 
	//						our belief in the state of the world
	//					- Represents the output of an inference 
	// Internal nodes:	- May be directly observed or inferred
	//					- May correspond to a real thing or state in the world, an internal state of our agent
	//						or an abstract condition
	//					- only externally queried as part of training or editor workflows
	// Leaves:			- Base nodes with only prior probability 'below' them
	//					- Represent directly observed states
	//					- Represent real testable states of the world or the internal state of our agent
	//
	// Leaves and internal nodes are collectively classified as Evidence Nodes
	// currently all probability calculation is done on floats, a later improvement may be to use doubles
	class AWFUL_BBN_API BayesianBeliefNetwork
	{
	private:

	public:
		using NodeType = BBN_Node;
		// simple pointer, but probably want this to be a shared pointer or just a pool item handle
		using NodeHandle = NodeStoreType::ItemHandle;

		BayesianBeliefNetwork()
			: mRoots()
			, mNodes()
			, mIdMap()
			, mDependencyMap()
		{
		}
		virtual ~BayesianBeliefNetwork()
		{
			DeconstructAll();
		}
	protected:
		friend class BBN_Accessor;
		friend class BBN_Mutator;

		// this should be a proper class exposing an iterator like we do in the GOAP
		using NodeList = std::vector<NodeHandle>;
		using VisitedList = std::unordered_set<BBN_Node::NodeKeyType>;


		//********************************************************
		// Construction support:
		// these should possibly be moved to a factory to ensure that 
		// the create/populate/add flow is respected through the use
		// of dedicated code in an authoring system. Using mutator and 
		// accessor attorney objects that can limit scope.
		// For now simple approach
		NodeHandle CreateRootNode(IdentifierType aID, float aPriorProb);
		NodeHandle CreateEvidenceNode(IdentifierType aID, float aPriorProb);

		// technically unneeded, but it's some syntactic sugar to ease the bookkeeping requirements of calling code
		void AddConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability);
		void AddConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild, float aConditionalProbability);
		void ApplyPriors(const ProbabilitySet& aPriors);

		void EditConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability);

		void RemoveNode(IdentifierType aID);
		void RenameNode(IdentifierType aOldID, IdentifierType aNewID);
		void RemoveConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild);
		void RemoveConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild);
		//********************************************************



		// TODO: Refactor this to cleaner lambda approach
		using VisitorCallback = void (BayesianBeliefNetwork::*)(NodeType&, VisitedList&);
		void VisitAllRoots(VisitorCallback aCallBack);

		NodeList& GetRoots() { return mRoots; }
		NodeList& GetNodes() { return mNodes; }
		NodeList& GetChildren(const NodeType& aNode);
		const NodeList& GetRoots() const { return mRoots; }
		const NodeList& GetNodes() const { return mNodes; }
		const NodeList& GetChildren(const NodeType& aNode) const;
		
		void ReadPriors(ProbabilitySet& aPriors) const;

		NodeHandle GetHandle(const IdentifierType aID);
		NodeType& GetNode(const NodeHandle aKey)
		{
			assert(aKey.IsValid());
			return mNodeStore.Edit(aKey);
		}
		const NodeHandle GetHandle(const IdentifierType aID) const;
		const NodeType& GetNode(const NodeHandle aKey) const
		{
			assert(aKey.IsValid());
			return mNodeStore.Read(aKey);
		}


		unsigned int mRevisionNumber = 0;

	private:
		//********************************************************
		// this should also be part of the part of the limited scope
		// authoring support attorney pattern
		NodeHandle CreateNode(IdentifierType aID, float aPriorProb, bool aIsRoot);
		//********************************************************

		void DeconstructAll();

		BayesianBeliefNetwork(const BayesianBeliefNetwork& rhs) = delete;
		const BayesianBeliefNetwork& operator=(const BayesianBeliefNetwork& rhs) = delete;

		using ParentToChildMap = std::vector<NodeList>;
		using IdToNodeMap = std::map<IdentifierType, NodeHandle>;

		NodeList mRoots;
		NodeList mNodes;

		IdToNodeMap mIdMap;
		ParentToChildMap mDependencyMap;

		NodeStoreType mNodeStore;
	};
}