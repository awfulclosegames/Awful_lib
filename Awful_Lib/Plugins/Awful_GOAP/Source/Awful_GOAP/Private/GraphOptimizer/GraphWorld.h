// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <algorithm>
#include "BasicTypes.h"
#include "DefaultFunctors.h"
#include "DefaultEdgeFinder.h"
#include "DefaultNodeFinder.h"

// TODO:
//  one particularly problematic thing is that I assume an interface for the node type, so I implicitly need
//    to derive from a base class. I should make this explicit, either a required base class or amore proper interface

namespace Awful
{
	namespace GO
	{
		class BaseGraphWorld
		{
		protected:
			void SetGuid(BaseGraphNode& aNode, PathGUID aGuid) const { aNode.mGUID = aGuid; }
		};


		template <class T>
		class WorldIteratorSupport
		{
		public:
			using iterator = typename T::const_iterator;



			iterator begin() { return mWorldData.begin(); }
			iterator end() { return mWorldData.end(); }

		private:
			template<class A, class B, class C, class D>
			friend class GraphWorld;

			WorldIteratorSupport(const T& aWorld)
				: mWorldData(aWorld)
			{
			}

			const T& mWorldData;
		};



		// TODO:
		// This looks like a good opportunity for (more?) CRTP?
		//
		// Storage and access for the graph, the world within which optimization, pathing, and queries are performed
		// Specialize on the node type and the method for acquiring nodes and edges
		template
			<
			class NodeType,
			class NodeFinder = DefaultNodeFinder<NodeType>,
			class EdgeFinder = DefaultEdgeFinder<NodeType>,
			class OnPop = DefaultGNDNOnPop<NodeType>
			>
		class GraphWorld : public BaseGraphWorld
		{
		protected:
			using WorldDataType = typename NodeFinder::NodeCollectionType;

		public:
			using Node = NodeType;
			using Edge = typename NodeType::Edge;
			using ReferencePointType = typename Edge::ReferencePointType;
			using EdgeIteratorSupport = typename EdgeFinder::IteratorSupport;
			using WorldIterator = WorldIteratorSupport<WorldDataType>;

			NodeType& AddNode();

			const Edge* LinkNodes(PathGUID aFrom, PathGUID aTo);

			NodeType& EditNearestNode(const ReferencePointType& aPoint);
			const NodeType& FindNearestNode(const ReferencePointType& aPoint) const;

			const NodeType& GetNode(PathGUID aGUID) const;

			NodeType& EditNode(PathGUID aGUID);

			bool IsEmpty() const { return (int)mWorldData.empty(); }
			unsigned int Size() const { return (int)mWorldData.size(); }

			EdgeIteratorSupport GetEdges(const NodeType& aNode) const { return mEdgeFinder(aNode); }
			void ProcessPoppedNode(PathGUID aFrom, PathGUID aCurrent) const;

			NodeFinder& GetNodeFinder()
			{
				return mNodeFinder;
			}

			EdgeFinder& GetEdgeFinder()
			{
				return mEdgeFinder;
			}

			const EdgeFinder& GetEdgeFinder() const
			{
				return mEdgeFinder;
			}

			WorldIterator Iterate() const { return WorldIterator(GetNodes()); }

		protected:

			WorldDataType& GetNodes() { return mWorldData; }
			const WorldDataType& GetNodes() const { return mWorldData; }

		private:
			//template <class T>
			friend class WorldIteratorSupport<WorldDataType>;

			WorldDataType mWorldData;
			NodeFinder mNodeFinder;
			EdgeFinder mEdgeFinder;
			OnPop myOnPop;


			static Node sDefault;
		};




		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		typename GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::Node  GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::sDefault;


		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		NodeType& GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::AddNode()
		{
			// this is is simple but it's unclear that it's correct, since we probably need
			// the guid to be monotonic, but world size could conceivably shrink, also meaning guids could be reused

			int count = (int)mWorldData.size();
			NodeType tempNode;
			mWorldData.push_back(tempNode);
			NodeType& resultNode = mWorldData.back();
			SetGuid(resultNode, count);

			// should add a callback to support custom node insertion semantics

			return resultNode;
		}


		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		const typename GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::Edge* GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::LinkNodes(PathGUID aFrom, PathGUID aTo)
		{
			// TODO:
			// this needs a more robust notion of guid to storage location. Perhaps a remap table or a dequeue.
			// Currently this works fine as long as we don't dynamically add and delete these things
			// so dynamism means recomputing the world data right now. 
			if (mWorldData.size() <= std::max(aFrom, aTo))
			{
				return nullptr;
			}
			// should add a callback to support custom edge semantics
			NodeType& nodeFrom = mWorldData[aFrom];
			return &nodeFrom.CreateEdge(aTo);
		}


		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		NodeType& GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::EditNearestNode(const ReferencePointType& aPoint)
		{
			return mNodeFinder(mWorldData, aPoint);
		}

		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		const NodeType& GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::FindNearestNode(const typename GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::ReferencePointType& aPoint) const
		{
			return const_cast<const NodeType&>(const_cast<GraphWorld*>(this)->EditNearestNode(aPoint));
		}

		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		const NodeType& GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::GetNode(PathGUID aGUID) const
		{
			if (mWorldData.size() <= aGUID)
			{
				// Error case 
				static NodeType sDefault;
				return sDefault;
			}
			return mWorldData[aGUID];
		}

		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		NodeType& GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::EditNode(PathGUID aGUID)
		{
			if (mWorldData.size() <= aGUID)
			{
				// Error case 
				static NodeType sDefault;
				return sDefault;
			}
			return mWorldData[aGUID];
		}


		template <class NodeType, class NodeFinder, class EdgeFinder, class OnPop>
		void GraphWorld<NodeType, NodeFinder, EdgeFinder, OnPop>::ProcessPoppedNode(PathGUID aFrom, PathGUID aCurrent) const
		{
			// TODO:
			// I would like to fold the guid->node mapping lookup into the OnPop functor so that I could do it only when needed
			//
			// alternately I could add a node reference directly into the PathGuid
			mOnPop(GetNode(aFrom), GetNode(aCurrent));
		}



	}
}