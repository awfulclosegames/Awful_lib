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
		float Tau[2];
		float Beta[2];
		float A;
		float B;
		float C;
		float D;
		float Deltas[3];

		void Localize();
	};
	enum DeltaIndex
	{
		prev = 0,
		current = 1,
		next = 2,
	};

	static void GenerateCoeffisients(ParameterBlock& Block, TArray<FVector>& Coeffs);

	static void Populate(ParameterBlock& Block, const FKBSplinePoint& P0, const FKBSplinePoint& P1, const FKBSplinePoint& P2, const FKBSplinePoint& P3);
};
