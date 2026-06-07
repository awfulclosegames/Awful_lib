// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once


// support dynamic or loosely coupled graphs where the edges may need to be queried 
namespace Awful
{
	namespace GO
	{

		template <class NodeType>
		class DefaultEdgeFinder
		{
		public:
			class IteratorSupport
			{
			public:
				using EdgeIterator = typename NodeType::EdgeIterator;

				IteratorSupport(const NodeType& aNode)
					: mBegin(aNode.GetLinkBegin())
					, mEnd(aNode.GetLinkEnd())
				{
				}

				EdgeIterator begin() { return mBegin; }
				EdgeIterator end() { return mEnd; }

			private:

				EdgeIterator mBegin;
				EdgeIterator mEnd;
			};


			IteratorSupport operator()(const NodeType& aNode)const { return IteratorSupport(aNode); }
		};

	}
}