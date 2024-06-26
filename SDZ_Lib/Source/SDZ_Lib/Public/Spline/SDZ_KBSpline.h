// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SDZ_KBSpline_DataTypes.h"

#include "SDZ_KBSpline.generated.h"

/**
 * 
 */
UCLASS()
class SDZ_LIB_API USDZ_KBSpline : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable)
	static void AddSplinePoint(UKBSplineConfig* Config, FKBSplinePoint Point);

	UFUNCTION(BlueprintCallable)
	static void RemoveLastSplinePoint(UKBSplineConfig* Config);

	static void Reset(UKBSplineConfig* Config);

	UFUNCTION(BlueprintCallable)
	static void GetChord(UKBSplineConfig* Config, int SegmentID, FVector& outChord);

	UFUNCTION(BlueprintCallable)
	static void AddSegmentConstraint(UKBSplineConfig* Config, FKBSplineBounds Bound, int SegmentID);

	UFUNCTION(BlueprintCallable)
	static UKBSplineConfig* CreateSplineConfig(FVector Location);

	UFUNCTION(BlueprintCallable)
	static FKBSplineState PrepareForEvaluation(UKBSplineConfig* Config, int PointID = 1);

	UFUNCTION(BlueprintCallable)
	static FVector Sample(FKBSplineState State);

	UFUNCTION(BlueprintCallable)
	static FVector SampleExplicit(FKBSplineState State, float Completion);

	UFUNCTION(BlueprintCallable)
	static void DrawDebug(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State, FColor CurveColour = FColor::Blue, float Width = 0.0f);

	UFUNCTION(BlueprintCallable)
	static FKBSplineState Split(UKBSplineConfig* Config, const FKBSplineState State, float Alpha);

private:
#if !UE_BUILD_SHIPPING
	static void DrawDebugConstraints(AActor* Actor, const UKBSplineConfig* Config, FKBSplineState State);

#endif

};
