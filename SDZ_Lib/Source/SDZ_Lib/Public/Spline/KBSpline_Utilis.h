#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "SDZ_KBSpline_DataTypes.h"

class KBSplineUtils
{
public:
	bool Prepare(const UKBSplineConfig& Config, FKBSplineState& State);

	void GenerateCoeffisients(const FKBSplinePoint* Points, float a, float b, float c, float d, TArray<FVector>& Coeffs);

private:

};
