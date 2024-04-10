// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SDZ_KBSpline_DataTypes.generated.h"


USTRUCT(BlueprintType)
struct FKBSplinePoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = { 0.0f, 0.0f, 0.0f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Tau = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Beta = 0.5f;
};

USTRUCT(BlueprintType)
struct FKBSplineBounds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FromBoundMax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FromBoundMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ToBoundMax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ToBoundMin;
};

// should this be a class instead of struct so I don't need to copy 36 bytes all the time?
USTRUCT(BlueprintType)
struct FKBSplineState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int CurrentTraversalSegment = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize)
	TArray<FVector> PrecomputedCoefficients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Time = 0.0f;

	// ************************************************************************
	// SUPPLIMENTARY STATE
	// ************************************************************************
	// 
	// I don't love putting this here, but since the restriction may change the
	// tensioning I need to store it after success. Ideally I should split off 
	// core state and supplimentary state since this isn't needed for sampling
	float Tau[2];
	float Beta[2];

#if !UE_BUILD_SHIPPING
	float UndulationTimes[2] = { -1.0f, -1.0f };
	FVector OriginalCoeffs[4] = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
#endif
	FKBSplineState();
};


UCLASS(BlueprintType)
class UKBSplineConfig : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FKBSplinePoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<int, FKBSplineBounds> SegmentBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FKBSplinePoint OriginPoint;

	UKBSplineConfig() : Super() {}

	UKBSplineConfig(FVector Location);

	bool IsValidSegment(int ID) const { return ID > 0 && ID < ControlPoints.Num(); }

};

