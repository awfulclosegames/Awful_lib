// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <vector>
#include <cassert>
#include "Solver/ActionPlanningUnit.h"

namespace Awful
{
	namespace ActionSpaceSearch
	{


		class ActionLookup
		{
		public:
			ActionLookup()
			{
				Clear();
			}

			void Resize(unsigned int aSize)
			{
				mRedirect.resize(aSize);
				Clear();
			}
			using NodeType = ActionPlanningUnit;
			using EdgeType = ActionPlanningUnit::Edge;
			using NodeCollectionType = std::vector<NodeType>;

			// the nested type decelerations can be cleaned up... later
			NodeType& operator()(NodeCollectionType& worldData, const typename EdgeType::ReferencePointType& aPoint)
			{
				int remapped = mRedirect[aPoint.GetID()];
				assert(remapped >= 0);
				return worldData[remapped];
			}

			void RecordMapping(const NodeType& aNode)
			{
				mRedirect[aNode.GetLocator().GetID()] = aNode.GetInternalGUID();
			}

			int Map(int aFrom) const { return mRedirect[aFrom]; }
		private:
			void Clear()
			{
				for (auto& index : mRedirect)
				{
					index = -1;
				}
			}
			std::vector<int> mRedirect;
		};

	}

}