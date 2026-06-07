// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once

namespace Awful
{
	namespace GO
	{

		// a type for abstracting the relationship between a node and a locator or referencer. In a spatial case
		// this would be a locator that could be within or near a spatial region. In a logical graph this could be 
		// any arbitrary value used to identify a node. For instance if this were a graph of temperature ranges, the 
		// reference type could represent a specific temperature
		class NullReferenceType
		{
		public:
			float GetCost() const { return 0.0f; }
		};


		// NOTE: there is an assumption that the base reference type has a GetCost() function
		// these implied interfaces (interface by convention) architectures are becoming a bit 
		// ubiquitous. I think I'd like to rework this at some point
		template<class BaseReferenceType = NullReferenceType>
		class ReferencePoint : public BaseReferenceType
		{
		public:

			float GetReferenceCost() const { return BaseReferenceType::GetCost(); }

			ReferencePoint& operator=(const BaseReferenceType& other)
			{
				*(static_cast<BaseReferenceType*>(this)) = other;
				return *this;
			}
		private:
		};

	}
}