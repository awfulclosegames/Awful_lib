// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once


namespace Awful
{
	namespace GO
	{
		// Goes Nowhere, Does Nothing
		template <class StackItem>
		class DefaultGNDNOnPop
		{
		public:
			void operator()(const StackItem& aFrom, const StackItem& aCurrent) const {}
		};
	}
}