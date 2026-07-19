#pragma once

#ifdef UnrealBuild

#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "Utilities/RosettaStone.h"

using Vec3f = FVector;

#define ASSERT(expr) check(expr);
#else
// Standard C++
#include <assert.h>

namespace Awful
{
	template<typename T>
	struct Vector3
	{
	public:
		Vector3(T X,  T Y, T Z)
			:mData( { X, Y, Z })
		{
		}

		Vector3(T aSplat)
			:mData( { aSplat,aSplat,aSplat })
		{
		}

		T operator[](short aIndex)const { return mData.Indexed[aIndex]; }

		T& operator[](short aIndex) { return mData.Indexed[aIndex]; }

		Vector3& operator=(const Vector3& aRHS)
		{
			mData.X = aRHS.mData.X; mData.Y = aRHS.mData.Y; mData.Z = aRHS.mData.Z;
			return *this;
		}
		Vector3& operator+=(const Vector3& aRHS)
		{
			mData.X += aRHS.mData.X; mData.Y += aRHS.mData.Y; mData.Z += aRHS.mData.Z;
			return *this;
		}
		Vector3& operator-=(const Vector3& aRHS)
		{
			mData.X -= aRHS.mData.X; mData.Y -= aRHS.mData.Y; mData.Z -= aRHS.mData.Z;
			return *this;
		}
		Vector3& operator*=(const Vector3& aRHS)
		{
			mData.X *= aRHS.mData.X; mData.Y *= aRHS.mData.Y; mData.Z *= aRHS.mData.Z;
			return *this;
		}
		Vector3& operator/=(const Vector3& aRHS)
		{
			mData.X /= aRHS.mData.X; mData.Y /= aRHS.mData.Y; mData.Z /= aRHS.mData.Z;
			return *this;
		}

		Vector3 operator+( const Vector3& aRHS) const
		{
			return Vector3(mData.X + aRHS.mData.X, mData.Y + aRHS.mData.Y, mData.Z + aRHS.mData.Z);
		}
		Vector3 operator-( const Vector3& aRHS) const
		{
			return Vector3(mData.X - aRHS.mData.X, mData.Y - aRHS.mData.Y, mData.Z - aRHS.mData.Z);
		}
		Vector3 operator*(const Vector3& aRHS) const
		{
			return Vector3(mData.X * aRHS.mData.X, mData.Y * aRHS.mData.Y, mData.Z * aRHS.mData.Z);
		}
		Vector3 operator/(const Vector3& aRHS) const
		{
			return Vector3(mData.X / aRHS.mData.X, mData.Y / aRHS.mData.Y, mData.Z / aRHS.mData.Z);
		}

		T SquaredLength() const { return (mData.X * mData.X) + (mData.Y * mData.Y) + (mData.Z * mData.Z); }

		bool operator==(const Vector3& aRHS) const { return mData.X == aRHS.mData.X && mData.Y == aRHS.mData.Y && mData.Z == aRHS.mData.Z; }
		bool operator!=(const Vector3& aRHS) const {return !(*this == aRHS);
	}
	private:
		union
		{
			struct
			{
				T X;
				T Y;
				T Z;
			};
			T Indexed[3];
		} mData;

	};
}

using Vec3f = Awful::Vector3<float>;

#define ASSERT(expr) assert(expr)
#endif

#define NO_COPY(C)				\
  private:						\
	C( const C& );				\
	C& operator=( const C& );	\
