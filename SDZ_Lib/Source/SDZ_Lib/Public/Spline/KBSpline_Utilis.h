#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "SDZ_KBSpline_DataTypes.h"

class KBSplineUtils
{
public:


	static bool Prepare(const UKBSplineConfig& Config, FKBSplineState& State);



private:

	struct ParameterBlock
	{
		FVector RawPoints[4];
		FVector Points[4];
		FVector Coeffs[4];
		float Tau[2];
		float Beta[2];
		float A;
		float B;
		float C;
		float D;
		float Deltas[3];

		void Localize();
		void AlignToX();
	};
	enum DeltaIndex
	{
		prev = 0,
		current = 1,
		next = 2,
	};

	static void Populate(ParameterBlock& Block, const FKBSplinePoint& P0, const FKBSplinePoint& P1, const FKBSplinePoint& P2, const FKBSplinePoint& P3);
	static void GenerateCoeffisients(FVector points[4], ParameterBlock& Block, FVector Coeffs[4]);
	static void ComputeUndulationTimes(float UndulationTimes[2], const ParameterBlock& Block);
	static bool RestrictToBounds(const float UndulationTimes[2], const ParameterBlock& Block);

	// wierd that there doesn't seem to be a built in function
	static void BoundQuadraticRoots(float A, float B, float X, float& Root0, float& Root1);
};
