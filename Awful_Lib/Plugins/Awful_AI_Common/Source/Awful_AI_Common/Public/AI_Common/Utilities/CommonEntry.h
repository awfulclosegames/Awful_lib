#pragma once

#include <vector>
#include "AI_Common/Platform/RosettaStone.h"

namespace Awful
{

	class CommonEntry
	{
	public:
		CommonEntry() {}
		CommonEntry(const Vec3f& aPos) : m_Pos(aPos) {}
		const Vec3f& GetPos() const { return m_Pos; }
		void SetPos(const Vec3f& aNewVal) { m_Pos = aNewVal; }

	private:
		Vec3f m_Pos = Vec3f(0.0f);
	};

	using CommonWorkingSet = std::vector<CommonEntry*>;


	// naive implementation of a generic entry
	// assumes that the payload type is something small like 
	// an int, or a pointer.
	// If we want to support more general wrapping of classes than 
	// this will need some more sophistication
	template<typename PAYLOAD_T>
	class GenericEntry : public CommonEntry
	{
	public:
		GenericEntry() : CommonEntry() {}
		GenericEntry(PAYLOAD_T aPayload)
			: CommonEntry()
			, m_Payload(aPayload)
		{
		}
		GenericEntry(const Vec3f& aPos, PAYLOAD_T aPayload)
			: CommonEntry(aPos)
			, m_Payload(aPayload)
		{
		}

		PAYLOAD_T GetPayload() const { return m_Payload; }
		void SetPayload(PAYLOAD_T aPayload) { m_Payload = aPayload; }

		class WorkingSetAdapter
		{
		public:
			WorkingSetAdapter(CommonWorkingSet& aWorkingSet)
				: m_WorkingSet(aWorkingSet)
			{
			}

			class Iterator : public CommonWorkingSet::iterator
			{
			public:
				// just want to force the cast, so shadow the operator
				GenericEntry<PAYLOAD_T>* operator*() { return reinterpret_cast<GenericEntry<PAYLOAD_T>*>(CommonWorkingSet::iterator::operator*()); }
			};

			Iterator begin() { return Iterator(m_WorkingSet.begin()); }
			Iterator end() { return Iterator(m_WorkingSet.end()); }

		private:
			CommonWorkingSet& m_WorkingSet;
		};

	private:
		PAYLOAD_T m_Payload;
	};


}