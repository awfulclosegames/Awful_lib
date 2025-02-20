// Copyright Strati D. Zerbinis, 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AC_KBSpline_DataTypes.h"

#include "AC_KBSpline.generated.h"

/**
 * 
 */
UCLASS()
class AWFUL_SPLINEMOVEMENT_API UAC_KBSpline : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="Spline Movement")
	static int AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static void RemoveLastSplinePoint(UKBSplineConfig* Config);

	static void Reset(UKBSplineConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static void GetChord(UKBSplineConfig* Config, int SegmentID, FVector& outChord);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static void AddSegmentConstraint(UKBSplineConfig* Config, FKBSplineBounds Bound, int SegmentID);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static UKBSplineConfig* CreateSplineConfig(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static FKBSplineState PrepareForEvaluation(UKBSplineConfig* Config, int PointID = 1);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static FVector ComputeTangent(FKBSplineState State);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static FVector ComputeTangentExplicit(FKBSplineState State, float Time);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static FVector Sample(FKBSplineState State);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static FVector SampleExplicit(FKBSplineState State, float Completion);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	static void DrawDebug(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State, FColor CurveColour = FColor::Blue, float Width = 0.0f, float DisplayTime = 1.0f);

private:
#if !UE_BUILD_SHIPPING
	static void DrawDebugConstraints(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State);

#endif

};
