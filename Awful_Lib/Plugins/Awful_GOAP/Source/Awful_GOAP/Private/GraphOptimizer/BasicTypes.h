// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <future>


// consider spreading these out into their own files maybe?

namespace Awful
{
	namespace GO
	{
		using PathGUID = unsigned int;
		static constexpr PathGUID sNULL_Node = 0;
		static const PathGUID sINVALID_Node = -1;


		struct GraphOptimizerConfig
		{
			enum Flags
			{

			};

			GraphOptimizerConfig() {}

			GraphOptimizerConfig(Flags aFlag)
			{
			}
			GraphOptimizerConfig(const float duration)
				: mMaxDuration(duration)
			{
			}

			float GetMaxDuration() const { return mMaxDuration; }

		private:
			float mMaxDuration = -1.0f;
		};



		class JobToken
		{
		public:
			static const int sInvalidReference = -1;
			// copy constructor for futures
			JobToken(const JobToken& rhs)
				: mReferenceNumber(rhs.mReferenceNumber)
				, mFuture(rhs.mFuture)
			{
			}

			const std::shared_future<bool>& getFuture() const { return mFuture; }
			const bool readFuture() { return mFuture.get(); }
			bool operator==(const JobToken& aRHS) const { return mReferenceNumber == aRHS.mReferenceNumber; }
			bool operator!=(const JobToken& aRHS) const { return !(*this == aRHS); }
			bool operator<(const JobToken& aRHS) const { return mReferenceNumber < aRHS.mReferenceNumber; }
			bool operator>(const JobToken& aRHS) const { return aRHS < (*this); }
			bool operator<=(const JobToken& aRHS) const { return !(*this > aRHS); }
			bool operator>=(const JobToken& aRHS) const { return !(*this < aRHS); }

		private:
			// todo split out the state management
			template <class A, class B, class C, class D, class E>
			friend class GraphCoreSolver;

			JobToken()
				: mReferenceNumber(sInvalidReference)
				, mFuture()
			{
			}


			JobToken(int aReferenceNumber)
				: mReferenceNumber(aReferenceNumber)
				, mFuture()
			{
			}

			int mReferenceNumber;
			std::shared_future<bool> mFuture;
		};


		class BaseGraphNode
		{
		public:
			BaseGraphNode()
				: mGUID(sINVALID_Node)
			{
			}
			const PathGUID& GetInternalGUID() const { return mGUID; }
		private:
			friend class BaseGraphWorld;
			PathGUID mGUID;
		};


		class DefaultSequenceBase
		{
		};

	}
}