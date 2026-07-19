// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#include "BayesianNetwork/Internal/BayesianBeliefNetwork.h"

namespace Awful_BeliefNet
{
	void BayesianBeliefNetwork::VisitAllRoots(VisitorCallback aCallBack)
	{
		VisitedList visited;
		for (auto nodeHandle : mRoots)
		{
			// some static function for converting pointers to references
			// might be cleaner here
			(this->*aCallBack)(GetNode(nodeHandle), visited);
		}

	}

	void BayesianBeliefNetwork::DeconstructAll()
	{
		mRevisionNumber++;
		mRoots.clear();
		for (auto current : mNodes)
		{
			mNodeStore.Release(current);
		}
		mNodes.clear();
		mDependencyMap.clear();
	}

	BayesianBeliefNetwork::NodeList& BayesianBeliefNetwork::GetChildren(const BayesianBeliefNetwork::NodeType& aNode)
	{
		return mDependencyMap[aNode.GetKey()];
	}

	const BayesianBeliefNetwork::NodeList& BayesianBeliefNetwork::GetChildren(const BayesianBeliefNetwork::NodeType& aNode) const
	{
		return mDependencyMap[aNode.GetKey()];
	}

	void BayesianBeliefNetwork::ReadPriors(ProbabilitySet& aPriors) const
	{
		for (NodeHandle currentHandle : mNodes)
		{
			const BBN_Node& currentNode = GetNode(currentHandle);
			aPriors.Add({ currentNode.GetIdentifier(), currentNode.GetPriorProbability() });
		}
	}


	BayesianBeliefNetwork::NodeHandle BayesianBeliefNetwork::GetHandle(const IdentifierType aID)
	{
		auto result = mIdMap.find(aID);
		if (result == mIdMap.end())
		{
			return NodeHandle();
		}
		return result->second;
	}

	const BayesianBeliefNetwork::NodeHandle BayesianBeliefNetwork::GetHandle(const IdentifierType aID) const
	{
		auto result = mIdMap.find(aID);
		if (result == mIdMap.end())
		{
			return NodeHandle();
		}
		return result->second;
	}



	//********************************************************
	// these should possibly be moved to private to ensure that 
	// the create/populate/add flow is respected through the use
	// of dedicated code in an authoring system. Using mutator and 
	// accessor attorney objects that can limit scope.
	// For now simple approach
	BayesianBeliefNetwork::NodeHandle BayesianBeliefNetwork::CreateRootNode(IdentifierType aID, float aPriorProb)
	{
		return CreateNode(aID, aPriorProb, true);
	}

	BayesianBeliefNetwork::NodeHandle BayesianBeliefNetwork::CreateEvidenceNode(IdentifierType aID, float aPriorProb)
	{
		return CreateNode(aID, aPriorProb, false);
	}

	// not considered performance critical since this is just a helper for authoring, so we can afford to do the lookups separately and not worry about the extra overhead.
	void BayesianBeliefNetwork::AddConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability)
	{
		auto parentHandle = GetHandle(aParent);
		auto childHandle = GetHandle(aChild);
		if (parentHandle.IsValid() && childHandle.IsValid())
		{
			AddConditionalProbability(parentHandle, childHandle, aConditionalProbability);
		}
		else
		{
			// assuming this is an error conditions. Probably is since we're currently not expecting a lot of dynamic structural 
			// changes to the network.
			assert(false);
		}
	}

	void BayesianBeliefNetwork::AddConditionalProbability(const BayesianBeliefNetwork::NodeHandle& aParent, const BayesianBeliefNetwork::NodeHandle& aChild, float aConditionalProbability)
	{
		mRevisionNumber++;

		BBN_Node& resolvedParent = GetNode(aParent);
		BBN_Node& resolvedChild = GetNode(aChild);

#ifdef AC_BBN_DEBUG_
		for (NodeHandle current : mDependencyMap[resolvedParent.GetKey()])
		{
			assert(current != aParent);
		}

#endif
		mDependencyMap[resolvedParent.GetKey()].push_back(aChild);
		resolvedChild.AddConditionalProbability(&resolvedParent, aConditionalProbability);

	}

	void BayesianBeliefNetwork::ApplyPriors(const ProbabilitySet& aPriors)
	{
		// this is not expected to be a common operation so it's not optimized. If this occurs frequently,
		// then we should add an identifier to node handle map
		for (NodeHandle currentHandle : mNodes)
		{
			BBN_Node& currentNode = GetNode(currentHandle);
			auto result = aPriors.Find(currentNode.GetIdentifier());
			if (aPriors.Valid(result))
			{
				currentNode.SetPriorProbability(aPriors.GetValue(result));
			}
		}
	}

	BayesianBeliefNetwork::NodeHandle BayesianBeliefNetwork::CreateNode(IdentifierType aID, float aPriorProb, bool aIsRoot)
	{
		mRevisionNumber++;

		NodeHandle newNodeHandel = mNodeStore.Acquire();
		if (newNodeHandel.IsValid())
		{
			BBN_Node& current = mNodeStore.Edit(newNodeHandel);
			current.SetIdentifier(aID);
			current.SetPriorProbability(aPriorProb);
			BBN_Node::NodeKeyType currentKey = static_cast<BBN_Node::NodeKeyType>(mNodes.size());
			current.SetKey(currentKey);
			mNodes.push_back(newNodeHandel);
			// get ready with a dependency list for this node, even if it's empty, so that we can just index into it when we need to add 
			// dependencies
			mDependencyMap.emplace_back();
			BBN_Node::NodeStateFlags typeFlag = BBN_Node::NodeStateFlags::EVIDENCE;
			if (aIsRoot)
			{
				mRoots.push_back(newNodeHandel);
				typeFlag = BBN_Node::NodeStateFlags::ROOT;
			}
			current.SetFlagValue(typeFlag);

			mIdMap.emplace(aID, newNodeHandel);
		}
		return newNodeHandel;
	}

	void BayesianBeliefNetwork::EditConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild, float aConditionalProbability)
	{
		// does not alter the revision number as it does not alter the structure of the network
		auto parentHandle = GetHandle(aParent);
		auto childHandle = GetHandle(aChild);

		// we do not want to proceed if either of these is invalid. This should never be the case, but if either one is invalid
		// it renders the operation invalid
		if (!(parentHandle.IsValid() || childHandle.IsValid()))
		{
			return;
		}

		BBN_Node& resolvedParent = GetNode(parentHandle);
		BBN_Node& resolvedChild = GetNode(childHandle);
		resolvedChild.ChangeConditionalProbability(&resolvedParent, aConditionalProbability);
	}


	// Rename a node: update its identifier and the id-map so all existing CPT parent-pointers
	// (raw BBN_Node*) remain valid thanks to ChunkedPool reference-stability.
	void BayesianBeliefNetwork::RenameNode(IdentifierType aOldID, IdentifierType aNewID)
	{
		auto resolvedHandle = GetHandle(aOldID);
		if (!resolvedHandle.IsValid())
		{
			return;
		}

		// Reject if a node with the new name already exists
		auto existingHandle = GetHandle(aNewID);
		if (existingHandle.IsValid())
		{
			return;
		}

		BBN_Node& node = GetNode(resolvedHandle);
		node.SetIdentifier(aNewID);

		mIdMap.erase(aOldID);
		mIdMap.emplace(aNewID, resolvedHandle);

		mRevisionNumber++;
	}


	// this is the worst function. Complex, expensive, and awkward. This is not expected to be frequent or even
	// performed outside of tool chains. Probably a good refactor to move this into an editor/factory class
	// TODO:
	// another simplification would be to use NodeHandle more generally than SequenceKey, since this would still be
	// stable under swap
	void BayesianBeliefNetwork::RemoveNode(IdentifierType aID)
	{
		mRevisionNumber++;
		auto resolvedHandle = GetHandle(aID);
		if (!resolvedHandle.IsValid())
		{
			return;
		}
		BBN_Node& toRemove = GetNode(resolvedHandle);

		// this is going to be rough!
		auto toRemoveKey = toRemove.GetKey();
		// clear it out of the roots. Not ordered, don't care
		if (toRemove.TestFlagValue(BBN_Node::ROOT))
		{
			for (auto iter = mRoots.begin(); iter != mRoots.end(); ++iter)
			{
				if (*iter == resolvedHandle)
				{
					mRoots.erase(iter);
				}
			}
		}

		// remove any references from me in the dependency map
		for (auto& parent : toRemove.GetCPT())
		{
			auto parentKey = parent.node->GetKey();
			if (parentKey < mDependencyMap.size())
			{
				auto& children = mDependencyMap[parentKey];
				for (auto iter = children.begin(); iter != children.end(); ++iter)
				{
					if (*iter == resolvedHandle)
					{
						auto foundChild = mDependencyMap[parentKey].erase(iter);
						break;
					}
				}
			}
		}

		// remove me from the my children
		for (auto child : mDependencyMap[toRemoveKey])
		{
			if (child.IsValid())
			{
				BBN_Node& currentChild = GetNode(child);
				currentChild.RemoveConditionalProbability(&toRemove);
			}
		}
		mDependencyMap[toRemoveKey].clear();

		// we want to remove it from the nodes list while preserving order (at least as much as possible) 
		// so we swap it with the last one and then update the affected keys
		auto toSwapHandle = mNodes[mNodes.size() - 1];
		BBN_Node& toSwap = GetNode(toSwapHandle);
		auto toSwapKey = toSwap.GetKey();
		auto toSwapNewKey = toRemoveKey;

		// then swap these in the dependency map and the node list
		mNodes[toRemoveKey] = mNodes[toSwapKey];
		mDependencyMap[toRemoveKey] = mDependencyMap[toSwapKey];
		mNodes.pop_back();
		mDependencyMap.pop_back();
		toSwap.SetKey(toSwapNewKey);

		// finally release the handle
		mNodeStore.Release(resolvedHandle);
	}


	void BayesianBeliefNetwork::RemoveConditionalProbability(const IdentifierType& aParent, const IdentifierType& aChild)
	{
		auto parentHandle = GetHandle(aParent);
		auto childHandle = GetHandle(aChild);

		if (!(parentHandle.IsValid() || childHandle.IsValid()))
		{
			return;
		}

		RemoveConditionalProbability(parentHandle, childHandle);
	}

	void BayesianBeliefNetwork::RemoveConditionalProbability(const NodeHandle& aParent, const NodeHandle& aChild)
	{
		BBN_Node& resolvedParent = GetNode(aParent);
		BBN_Node& resolvedChild = GetNode(aChild);

		mRevisionNumber++;

		auto parentKey = resolvedParent.GetKey();
		if (parentKey >= 0 && parentKey < mDependencyMap.size())
		{
			auto& children = mDependencyMap[parentKey];
			for (auto iter = children.begin(); iter != children.end(); ++iter)
			{
				if (*iter == aChild)
				{
					auto foundChild = mDependencyMap[parentKey].erase(iter);
					break;
				}
			}
		}

		resolvedChild.RemoveConditionalProbability(&resolvedParent);
	}

	//********************************************************

}