// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "BasicTypes.h"
#include "ReferencePoint.h"
#include "GraphEdge.h"


// the basic unit of the graph that we are optimizing a sequence or ordering on
// 
// open question:
//	I use a CRTP structure on the node. This eliminates some of the virtuals 
//  but at the cost of more of an interface by convention structure making it harder to use and 
//	more reliant on documentation 
//  Is this the best approach? should I rework this for a more explicit structure

namespace Awful
{
	namespace GO
	{
		class BaseGraphWorld;


		template <class ReferenceType, class Derived>
		class GraphNode : public BaseGraphNode
		{
		private:
			using EdgeType = GraphEdge<ReferenceType>;
			using Edges = std::vector<EdgeType>;
		public:
			using Edge = EdgeType;
			using EdgeIterator = typename Edges::const_iterator;
			using ReferencePointType = ReferenceType;

			GraphNode()
				: BaseGraphNode()
			{
			}

			virtual ~GraphNode() {}

			float GetTraversalCost() const
			{
				return Upcast().ComputeTraversalCost();
			}

			EdgeIterator GetEdgeBegin() const { return mEdges.begin(); }
			EdgeIterator GetEdgeEnd() const { return mEdges.end(); }

			const Edge* GetEdge(unsigned int aIndex)const;

			float DistanceToPoint(const ReferencePointType& aPoint) const
			{
				return Upcast().ComputeDistanceToPoint(aPoint);
			}

			// It would be great to hide this creation method, but since it is bound to the template types 
			// I'd have to get much cleverer to avoid this. I'll think about it mode
			Edge& CreateEdge(PathGUID aTarget);


		private:
			friend class BaseGraphWorld;
			const Derived& Upcast() const { return *reinterpret_cast<const Derived*>(this); }

			Edges mEdges;
		};


		template <class ReferenceType, class Derived>
		typename GraphNode<ReferenceType, Derived>::Edge& GraphNode<ReferenceType, Derived>::CreateEdge(PathGUID aTarget)
		{
			// Improve this
			Edge newEdge(aTarget);
			mEdges.push_back(newEdge);
			return mEdges.back();
		}

		template <class ReferenceType, class Derived>
		const typename GraphNode<ReferenceType, Derived>::Edge* GraphNode<ReferenceType, Derived>::GetEdge(unsigned int aIndex)const
		{
			// better debugging support here
			if (aIndex >= mEdges.size())
			{
				return nullptr;
			}

			return &(mEdges[aIndex]);
		}


	}
}
