// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once


namespace Awful
{
	namespace GO
	{
		template <class NodeType>
		class HeuristicBase
		{
		public:
			using Node = NodeType;
			using Edge = typename NodeType::Edge;

			// I dislike the I/O param model, should probably rewrite this to return a results block
			virtual void ComputeStep(const Node& aCurrentNode, const Node& aNeighbourNode
				, const Node& aDestNode, const Edge& aEdge
				, float& aHeuristicCost) const = 0;

			virtual float ComputeEstimate(const Node& aCurrentNode, const Node& aDestNode) const = 0;
		};


		template <class NodeType>
		class NullHeuristicBase : public HeuristicBase<NodeType>
		{
		public:
			using BASE = HeuristicBase<NodeType>;
			virtual void ComputeStep(const BASE::Node& aCurrentNode, const BASE::Node& aNeighbourNode
				, const BASE::Node& aDestNode, const BASE::Edge& aEdge
				, float& aHeuristicCost) const override
			{
				aHeuristicCost = 0.0f;
			}

			virtual float ComputeEstimate(const BASE::Node& aCurrentNode, const BASE::Node& aDestNode) const override
			{
				return 0.0f;
			}
		};
	}
}