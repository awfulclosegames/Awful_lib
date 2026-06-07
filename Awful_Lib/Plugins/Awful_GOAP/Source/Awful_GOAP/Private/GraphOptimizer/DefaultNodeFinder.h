// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

namespace Awful
{
	namespace GO
	{

		template <class NodeTypeClass>
		class DefaultNodeFinder
		{
			using NodeType = NodeTypeClass;
		public:

			// Later on I should have optional override/specializations for different containers
			using NodeCollectionType = std::vector<NodeType>;

			// I don't love the nested type declarations in a template parameter. But I keep not getting around to fixing this

			// the reference point is an abstraction for how we identify a node is of interest. Spatially it could be a location
			// if the node represents a region of space that might contain it. 
			//
			// so the operator should be trying to find a node that is "near" or "encompassing" the target point for definitions
			// that are provided by the node type.
			// this is the default so let's act like this is space
			NodeType& operator()(NodeCollectionType& aWorldData, const typename NodeType::Edge::ReferencePointType& aPoint)
			{
				// I really should sort or prefilter here. But this is good enough for now, and I'll profile it more later
				NodeType* nearestRegion = &sDefault;
				float distance = FLT_MAX;
				for (NodeType& currentRegion : aWorldData)
				{
					// Broad phase/fine phase or an acceleration structure would help here. Common cases like regular grids
					// should probably provide custom implementations that are better suited
					// 
					// Add a configuration option to apply a custom solve using a user provided acceleration structure?
					float currentDistance = currentRegion.DistanceToPoint(aPoint);

					if (currentDistance <= FLT_EPSILON)
					{
						return currentRegion;
					}

					if (currentDistance < distance)
					{
						distance = currentDistance;
						nearestRegion = &currentRegion;
					}
				}
				return *nearestRegion;
			}
		private:

			static NodeType sDefault;
		};

		template <class NodeTyp>
		typename DefaultNodeFinder<NodeTyp>::NodeType DefaultNodeFinder<NodeTyp>::sDefault;
	}
}