// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include "ReferencePoint.h"

namespace Awful
{
	namespace GO
	{

		// a general structure for holding edge meta-information. Edges are assumed to be unidirectional
		// NOTE:
		//		Templated to avoid virtual calls on common functions
		//		Derive to add additional domain specific information (like spatial relationships)
		//		  though it might be nice to allow for adding domain specific metadata blocks to standard edges
		template <class ReferenceType = ReferencePoint<> >
		class GraphEdge
		{
		public:
			using ReferencePointType = ReferenceType;

			GraphEdge()
				: mTargetNode(sNULL_Node)
			{
			}

			GraphEdge(PathGUID aTargetID)
				: mTargetNode(aTargetID)
			{
			}

			GraphEdge(PathGUID aTargetID, ReferencePointType aRefernce)
				: mTargetNode(aTargetID)
				, mReferencePoint(aRefernce)
			{
			}

			GraphEdge(const GraphEdge& rhs)
				: mTargetNode(rhs.mTargetNode)
				, mReferencePoint(rhs.mReferencePoint)
			{
			}

			const ReferencePoint<ReferenceType>& GetConnectionReference() const { return mReferencePoint; }
			void SetConnectionReference(ReferencePointType& aRefPoint) { mReferencePoint = aRefPoint; }

			const PathGUID& GetEdgeTarget() const { return mTargetNode; }

			float GetTraversalCost() const { return mReferencePoint.GetReferenceCost(); }

		private:

			PathGUID mTargetNode;
			ReferencePoint<ReferenceType> mReferencePoint;
		};

	}
}